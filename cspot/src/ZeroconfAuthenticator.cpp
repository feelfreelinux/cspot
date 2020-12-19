#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2Tcpip.h>
#endif

#include "ZeroconfAuthenticator.h"
#include "JSONObject.h"
#include <sstream>

#ifdef _WIN32
#define addrinfo_t ADDRINFOA
#else
#define addrinfo_t struct addrinfo
#endif

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

    addrinfo_t hints, *server;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, std::to_string(serverPort).c_str(), &hints, &server);

    int sockfd = socket(server->ai_family,
                        server->ai_socktype, server->ai_protocol);
    bind(sockfd, server->ai_addr, server->ai_addrlen);
    listen(sockfd, 10);

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;

    // Make it discoverable for spoti clients
    registerZeroconf();

    for (;;)
    {
        int clientFd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
#ifdef _WIN32
        int iResult;
        u_long iMode = 1;
        iResult = ioctlsocket(clientFd, FIONBIO, &iMode);
        if (iResult != NO_ERROR)
        {
            perror("failed to  ioctlsocket(clientFd, FIONBIO, &iMode);");
            continue;
        };
#else
        if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
        {
            perror("failed to fcntl(clientFd, F_SETFL, O_NONBLOCK);");
            continue;
        };
#endif
        int readBytes = 0;
        std::vector<uint8_t> bufferVec(128);

        auto currentString = std::string();
        bool isAddUserRequest = false;

        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 30000;
        FD_ZERO(&set);
        FD_SET(clientFd, &set);

        for (;;)
        {

            int rv = select(clientFd + 1, &set, NULL, NULL, &timeout);
            if (rv == -1)
            {
                perror("select"); /* an error occured */
                break;
            }
            else if (rv == 0)
            {
                break;
            }
            else
            {
                readBytes = recv(clientFd, reinterpret_cast<char *>(bufferVec.data()), 128, 0);
            }

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

        printf("RQ: %s\n", currentString.c_str());

        if (isAddUserRequest)
        {

            printf("Got POST request!\n");
            JSONObject obj;
            obj["status"] = 101;
            obj["spotifyError"] = 0;
            obj["statusString"] = "ERROR-OK";
            auto jsonString = obj.toString();

            std::stringstream stream;
            stream << "HTTP/1.1 200 OK\r\n";
            stream << "Server: cspot\r\n";
            stream << "Content-type: application/json\r\n";
            stream << "Content-length:" << jsonString.size() << "\r\n";
            stream << "X-DDD: ADD USER\r\n";
            stream << "\r\n";
            stream << jsonString;

            auto response = stream.str();
            send(clientFd, response.c_str(), response.size(), 0);
#ifdef _WIN32
            closesocket(clientFd);
#else
            close(clientFd);
#endif

            return handleAddUser(currentString);
            // break;
        }
        else
        {

            printf("Got GET request!\n");
            std::string jsonInfo = buildJsonInfo();

            std::stringstream stream;
            stream << "HTTP/1.1 200 OK\r\n";
            stream << "Server: cspot\r\n";
            stream << "Content-type: application/json\r\n";
            stream << "Content-length:" << jsonInfo.size() << "\r\n";
            stream << "X-DDD: Ok, mocz2\r\n";
            stream << "\r\n";
            stream << jsonInfo;
            // Respond with player info
            auto response = stream.str();
            send(clientFd, response.c_str(), response.size(), 0);
#ifdef _WIN32
            closesocket(clientFd);
#else
            close(clientFd);
#endif
        }
    }
}

std::string ZeroconfAuthenticator::getParameterFromUrlEncoded(std::string data, std::string param)
{
    auto startStr = data.substr(data.find("&" + param + "=") + param.size() + 2, data.size());
    return urlDecode(startStr.substr(0, startStr.find("&")));
}

#ifdef ESP_PLATFORM
void ZeroconfAuthenticator::registerZeroconf()
{
    mdns_init();
    mdns_hostname_set("cspot");
    mdns_txt_item_t serviceTxtData[3] = {
        {"VERSION", "1.0"},
        {"CPath", "/"},
        {"Stack", "SP"}};
    mdns_service_add("cspot", "_spotify-connect", "_tcp", serverPort, serviceTxtData, 3);
}
#elif _WIN32

static std::wstring s2ws(const std::string &s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t *buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

void ZeroconfAuthenticator::registerZeroconf()
{
    DWORD size = 0;
    GetComputerNameEx(ComputerNameDnsHostname, nullptr, &size);
    std::vector<wchar_t> hostname(size);
    if (!GetComputerNameEx(ComputerNameDnsHostname, reinterpret_cast<LPSTR>(&hostname[0]), &size))
    {
        printf("Zeroconf: GetComputerNameEx() failed with error %u!", GetLastError());
        return;
    }

    // const auto domain            = record.replyDomain.isEmpty() ? L".local" : L"." + record.replyDomain.toStdWString();
    const auto qualifiedHostname = std::string(hostname.begin(), hostname.end()) + ".local";

    // auto service = record.serviceName.isEmpty() ? &hostname[0] : record.serviceName.toStdWString();
    // service += L".";
    // service += record.registeredType.toStdWString();
    // service += domain;

    constexpr int textPropertiesCount = 3;

    PCWSTR keys[textPropertiesCount] = {
        L"VERSION",
        L"CPath",
        L"Stack"};

    PCWSTR values[textPropertiesCount] = {
        L"1.0",
        L"/",
        L"SP"};

    auto instance = DnsServiceConstructInstance(
        L"_spotify-connect._tcp.local",
        s2ws(qualifiedHostname).c_str(), // A string that represents the name of the host of the service.
        nullptr,                         // A pointer to an IP4_ADDRESS structure that represents the service-associated IPv4 address.
        nullptr,                         // A pointer to an IP6_ADDRESS structure that represents the service-associated IPv6 address.
        serverPort,                      // A value that represents the port on which the service is running.
        0,                               // A value that represents the service priority.
        0,                               // A value that represents the service weight.
        textPropertiesCount,
        keys,
        values);
    // TODO: add
    DNS_SERVICE_REGISTER_REQUEST rq;
    rq.Version = DNS_QUERY_REQUEST_VERSION1;
    rq.InterfaceIndex = 0; // all interfaces
    rq.unicastEnabled = false;
    rq.pServiceInstance = instance;
    const auto ret = DnsServiceRegister(&rq, nullptr);
    DnsServiceFreeInstance(instance);

    if (ret == DNS_REQUEST_PENDING)
    {
        return;
    }

    printf("Zeroconf: DnsServiceRegister() failed with error %u!", ret);
}
#else
void ZeroconfAuthenticator::registerZeroconf()
{
    DNSServiceRef ref = NULL;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);
    TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    TXTRecordSetValue(&txtRecord, "CPath", 1, "/");
    TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    DNSServiceRegister(&ref, 0, 0, (char *)informationString, "_spotify-connect._tcp", NULL, NULL, htons(serverPort), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    TXTRecordDeallocate(&txtRecord);
}
#endif

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
