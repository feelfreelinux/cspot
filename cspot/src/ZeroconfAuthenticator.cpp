#include "ZeroconfAuthenticator.h"

ZeroconfAuthenticator::ZeroconfAuthenticator()
{
    srand((unsigned int)time(NULL));

    this->localKeys = std::make_unique<DiffieHellman>();

    this->serverPort = SERVER_PORT_MIN + (std::rand() % (SERVER_PORT_MAX - SERVER_PORT_MIN + 1));

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

                if (line.rfind("POST /", 0) == 0)
                {
                    isAddUserRequest = true;
                }
            }
        }


        if (isAddUserRequest)
        {
            // Empty json response
            auto response = responseHeader + std::string("{}");
            write(clientFd, response.c_str(), response.size());
            close(clientFd);

            handleAddUser(currentString);
            // break;
        }
        else
        {
            // Respond with player info
            auto response = responseHeader + buildJsonInfo();
            write(clientFd, response.c_str(), response.size());
            close(clientFd);
        }
    }
}

std::string ZeroconfAuthenticator::getParameterFromUrlEncoded(std::string data, std::string param) {
    
}

void ZeroconfAuthenticator::registerZeroconf()
{
    const char *service = "_spotify-connect._tcp";

    DNSServiceRef ref = NULL;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);
    TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    TXTRecordSetValue(&txtRecord, "CPath", 1, "/");
    TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    DNSServiceRegister(&ref, 0, 0, (char*) informationString, service, NULL, NULL, htons(serverPort), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    TXTRecordDeallocate(&txtRecord);
}

void ZeroconfAuthenticator::handleAddUser(std::string userData) {
    printf("Received addUser request\n");
    std::cout << userData << std::endl;
}


std::string ZeroconfAuthenticator::buildJsonInfo()
{
    // Encode publicKey into base64
    auto encodedKey = base64_encode(localKeys->publicKey.data(), localKeys->publicKey.size());
    cJSON *baseBody = cJSON_CreateObject();

    cJSON_AddNumberToObject(baseBody, "status", 101);
    cJSON_AddStringToObject(baseBody, "statusString", "OK");
    cJSON_AddStringToObject(baseBody, "version", (char *)protocolVersion);
    cJSON_AddNumberToObject(baseBody, "spotifyError", 0);
    cJSON_AddStringToObject(baseBody, "libraryVersion", (char *)swVersion);
    cJSON_AddStringToObject(baseBody, "accountReq", "PREMIUM");
    cJSON_AddStringToObject(baseBody, "brandDisplayName", (char *)brandName);
    cJSON_AddStringToObject(baseBody, "modelDisplayName", (char *)defaultDeviceName);
    cJSON_AddStringToObject(baseBody, "voiceSupport", "NO");
    cJSON_AddStringToObject(baseBody, "availability", "");
    cJSON_AddNumberToObject(baseBody, "productID", 0);
    cJSON_AddStringToObject(baseBody, "tokenType", "default");
    cJSON_AddStringToObject(baseBody, "groupStatus", "NONE");
    cJSON_AddStringToObject(baseBody, "resolverVersion", "0");
    cJSON_AddStringToObject(baseBody, "scope", "streaming,client-authorization-universal");
    cJSON_AddStringToObject(baseBody, "activeUser", "");
    cJSON_AddStringToObject(baseBody, "deviceID", (char *)deviceId);
    cJSON_AddStringToObject(baseBody, "remoteName", (char *)defaultDeviceName);
    cJSON_AddStringToObject(baseBody, "publicKey", encodedKey.c_str());
    cJSON_AddStringToObject(baseBody, "deviceType", "SPEAKER");

    char *body = cJSON_Print(baseBody);
    cJSON_Delete(baseBody);
    return std::string(body);
}