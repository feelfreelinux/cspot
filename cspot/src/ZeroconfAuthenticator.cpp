#include "ZeroconfAuthenticator.h"
#include "JSONObject.h"

ZeroconfAuthenticator::ZeroconfAuthenticator()
{
    srand((unsigned int)time(NULL));

    this->crypto = std::make_unique<Crypto>();
    this->crypto->dhInit();

    // @TODO: Maybe verify if given port is taken. We're running off pure luck rn
    this->serverPort = SERVER_PORT_MIN + (std::rand() % (SERVER_PORT_MAX - SERVER_PORT_MIN + 1));
}

std::shared_ptr<LoginBlob> ZeroconfAuthenticator::listenForRequests()
{

    printf("Starting zeroconf auth server at port %d\n", this->serverPort);

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE || SOCK_NONBLOCK;
    getaddrinfo(NULL, std::to_string(serverPort).c_str(), &hints, &server);

    int sockfd = socket(server->ai_family,
                        server->ai_socktype, server->ai_protocol);
    bind(sockfd, server->ai_addr, server->ai_addrlen);
    listen(sockfd, 10);

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;

    // Standard http response header
    auto responseHeader = std::string("HTTP/1.1 200 OK\r\nServer: cspot\r\nContent-type: application/json\r\n\r\n");

    // Make it discoverable for spoti clients
    registerZeroconf();

    for (;;)
    {
        int clientFd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        fcntl(clientFd, F_SETFL, O_NONBLOCK);
        int readBytes = 0;
        std::vector<uint8_t> bufferVec(128);

        auto currentString = std::string();
        bool isAddUserRequest = false;

        for (;;)
        {
            readBytes = read(clientFd, bufferVec.data(), 128);

            // Read entire response so lets yeeeet
            if (readBytes <= 0)
                break;

            currentString += std::string(bufferVec.data(), bufferVec.data() + readBytes);

            while (currentString.find("\r\n") != std::string::npos)
            {
                auto line = currentString.substr(0, currentString.find("\r\n"));
                currentString = currentString.substr(currentString.find("\r\n") + 2, currentString.size());

                // The only post is add User request
                if (line.rfind("POST /", 0) == 0)
                {
                    isAddUserRequest = true;
                }
            }
        }

        if (isAddUserRequest)
        {
            // Empty json response
            cJSON *baseBody = cJSON_CreateObject();

            cJSON_AddNumberToObject(baseBody, "status", 101);
            cJSON_AddNumberToObject(baseBody, "spotifyError", 0);
            cJSON_AddStringToObject(baseBody, "statusString", "ERROR-OK");

            // Get body
            char *body = cJSON_Print(baseBody);
            cJSON_Delete(baseBody);

            auto response = responseHeader + std::string(body);
            write(clientFd, response.c_str(), response.size());
            close(clientFd);

            return handleAddUser(currentString);
            // break;
        }
        else
        {
            // Respond with player info
            auto response = responseHeader + buildJsonInfo();
            write(clientFd, response.c_str(), response.size());
            usleep(500000);
            close(clientFd);
        }
    }
}

std::string ZeroconfAuthenticator::getParameterFromUrlEncoded(std::string data, std::string param)
{
    auto startStr = data.substr(data.find("&" + param + "=") + param.size() + 2, data.size());
    return urlDecode(startStr.substr(0, startStr.find("&")));
}

void ZeroconfAuthenticator::registerZeroconf()
{
    const char *service = "_spotify-connect._tcp";

#ifdef ESP_PLATFORM
    mdns_init();
    mdns_hostname_set("cspot");
    mdns_txt_item_t serviceTxtData[3] = {
        {"VERSION", "1.0"},
        {"CPath", "/"},
        {"Stack", "SP"}};
    mdns_service_add("cspot", "_spotify-connect", "_tcp", serverPort, serviceTxtData, 3);

#else
    DNSServiceRef ref = NULL;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);
    TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    TXTRecordSetValue(&txtRecord, "CPath", 1, "/");
    TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    DNSServiceRegister(&ref, 0, 0, (char *)informationString, service, NULL, NULL, htons(serverPort), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    TXTRecordDeallocate(&txtRecord);
#endif
}

std::shared_ptr<LoginBlob> ZeroconfAuthenticator::handleAddUser(std::string userData)
{
    // Get all urlencoded params
    auto username = getParameterFromUrlEncoded(userData, "userName");
    auto blobString = getParameterFromUrlEncoded(userData, "blob");
    auto clientKeyString = getParameterFromUrlEncoded(userData, "clientKey");
    auto deviceName = getParameterFromUrlEncoded(userData, "deviceName");

    // client key and bytes are urlencoded
    auto clientKeyBytes = crypto->base64Decode(clientKeyString);
    auto blobBytes = crypto->base64Decode(blobString);

    // Generated secret based on earlier generated DH
    auto secretKey = crypto->dhCalculateShared(clientKeyBytes);

    auto loginBlob = std::make_shared<LoginBlob>();

    std::string deviceIdStr = deviceId;

    loginBlob->loadZeroconf(blobBytes, secretKey, deviceIdStr, username);

    return loginBlob;
}

std::string ZeroconfAuthenticator::buildJsonInfo()
{
    // Encode publicKey into base64
    auto encodedKey = crypto->base64Encode(crypto->publicKey);

    JSONObject obj;
    obj["status"] = 101;
    obj["statusString"] = "OK";
    obj["version"] = protocolVersion;
    obj["spotifyError"] = 0;
    obj["libraryVersion"] = swVersion;
    obj["accountReq"] = "PREMIUM";
    obj["brandDisplayName"] = brandName;
    obj["modelDisplayName"] = defaultDeviceName;
    obj["voiceSupport"] = "NO";
    obj["availability"] = "";
    obj["productID"] = 0;
    obj["tokenType"] = "default";
    obj["groupStatus"] = "NONE";
    obj["resolverVersion"] = "0";
    obj["scope"] = "streaming,client-authorization-universal";
    obj["activeUser"] = "";
    obj["deviceID"] = deviceId;
    obj["remoteName"] = defaultDeviceName;
    obj["publicKey"] = encodedKey;
    obj["deviceType"] = "SPEAKER";
    return obj.toString();
}