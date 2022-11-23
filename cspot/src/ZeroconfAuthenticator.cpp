#include "ZeroconfAuthenticator.h"
#include "JSONObject.h"
#include <sstream>
#ifndef _WIN32
#include <sys/select.h>
#else
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "Logger.h"
#include "CspotAssert.h"

#ifdef _WIN32
struct mdnsd* ZeroconfAuthenticator::service = NULL;
#endif

ZeroconfAuthenticator::ZeroconfAuthenticator(authCallback callback, std::shared_ptr<bell::BaseHTTPServer> httpServer, std::string name, std::string deviceId, void *mdnsService) {
    this->gotBlobCallback = callback;
    srand((unsigned int)time(NULL));

    this->crypto = std::make_unique<Crypto>();
    this->crypto->dhInit();
    this->server = httpServer;
    this->name = name;
    this->deviceId = deviceId;

#ifdef _WIN32
    if (ZeroconfAuthenticator::service || mdnsService) {
        if (mdnsService) ZeroconfAuthenticator::service = (struct mdnsd*) mdnsService;
        return;
    }

    char hostname[128];
    gethostname(hostname, sizeof(hostname));

    struct sockaddr_in* host = NULL;
    ULONG size = sizeof(IP_ADAPTER_ADDRESSES) * 32;
    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*) malloc(size);
    int ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, 0, adapters, &size);

    for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter && !host; adapter = adapter->Next) {
        if (adapter->TunnelType == TUNNEL_TYPE_TEREDO) continue;
        if (adapter->OperStatus != IfOperStatusUp) continue;

        for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast;
            unicast = unicast->Next) {
            if (adapter->FirstGatewayAddress && unicast->Address.lpSockaddr->sa_family == AF_INET) {
                host = (struct sockaddr_in*)unicast->Address.lpSockaddr;
                BELL_LOG(info, "mdns", "mDNS on interface %s", inet_ntoa(host->sin_addr));
                mdnsService = mdnsd_start(host->sin_addr, false);
                break;
            }
        }
    }

    ZeroconfAuthenticator::service = (struct mdnsd*) mdnsService;
	CSPOT_ASSERT(ZeroconfAuthenticator::service, "can't start mDNS service");
    mdnsd_set_hostname(ZeroconfAuthenticator::service, hostname, host->sin_addr);
#endif
}

void ZeroconfAuthenticator::registerHandlers() {
    // Make it discoverable for spoti clients
    registerZeroconf();
    auto getInfoHandler = [this](std::unique_ptr<bell::HTTPRequest> request) {
        CSPOT_LOG(info, "Got request for info");
        bell::HTTPResponse response = {
            .connectionFd = request->connection,
            .status = 200,
            .body = this->buildJsonInfo(),
            .contentType = "application/json",
        };
        server->respond(response);
    };

    auto addUserHandler = [this](std::unique_ptr<bell::HTTPRequest> request) {
        BELL_LOG(info, "http", "Got request for adding user");
        bell::JSONObject obj;
        obj["status"] = 101;
        obj["spotifyError"] = 0;
        obj["statusString"] = "ERROR-OK";

        bell::HTTPResponse response = {
            .connectionFd = request->connection,
            .status = 200,
            .body = obj.toString(),
            .contentType = "application/json",
        };
        server->respond(response);

        auto correctBlob = this->getParameterFromUrlEncoded(request->body, "blob");
        this->handleAddUser(request->queryParams);
    };

    BELL_LOG(info, "cspot", "Zeroconf registering handlers");
    this->server->registerHandler(bell::RequestType::GET, "/spotify_info", getInfoHandler);
    this->server->registerHandler(bell::RequestType::POST, "/spotify_info", addUserHandler);
}

void ZeroconfAuthenticator::registerZeroconf()
{
#ifdef ESP_PLATFORM
	mdns_txt_item_t serviceTxtData[3] = {
		{"VERSION", "1.0"},
		{"CPath", "/spotify_info"},
		{"Stack", "SP"} };
	mdns_service_add(this->name.c_str(), "_spotify-connect", "_tcp", this->server->serverPort, serviceTxtData, 3);
#elif _WIN32
	const char *serviceTxtData[] = {
		"VERSION=1.0",
		"CPath=/spotify_info",
		"Stack=SP",
		NULL };
    mdnsd_register_svc(ZeroconfAuthenticator::service, this->name.c_str(), "_spotify-connect._tcp.local", this->server->serverPort, NULL, serviceTxtData);
#else
    const char* service = "_spotify-connect._tcp";
    DNSServiceRef ref = NULL;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);
    TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    TXTRecordSetValue(&txtRecord, "CPath", 13, "/spotify_info");
    TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    DNSServiceRegister(&ref, 0, 0, this->name.c_str(), service, NULL, NULL, htons(this->server->serverPort), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    TXTRecordDeallocate(&txtRecord);
#endif
}

std::string ZeroconfAuthenticator::getParameterFromUrlEncoded(std::string data, std::string param)
{
    auto startStr = data.substr(data.find("&" + param + "=") + param.size() + 2, data.size());
    return urlDecode(startStr.substr(0, startStr.find("&")));
}

void ZeroconfAuthenticator::handleAddUser(std::map<std::string, std::string>& queryData)
{
    // Get all urlencoded params
    auto username = queryData["userName"];
    auto blobString = queryData["blob"];
    auto clientKeyString = queryData["clientKey"];
    auto deviceName = queryData["deviceName"];

    // client key and bytes are urlencoded
    auto clientKeyBytes = crypto->base64Decode(clientKeyString);
    auto blobBytes = crypto->base64Decode(blobString);

    // Generated secret based on earlier generated DH
    auto secretKey = crypto->dhCalculateShared(clientKeyBytes);

    auto loginBlob = std::make_shared<LoginBlob>();

    std::string deviceIdStr = deviceId;

    loginBlob->loadZeroconf(blobBytes, secretKey, deviceIdStr, username);

    gotBlobCallback(loginBlob);
}

std::string ZeroconfAuthenticator::buildJsonInfo()
{
    // Encode publicKey into base64
    auto encodedKey = crypto->base64Encode(crypto->publicKey);

    bell::JSONObject obj;
    obj["status"] = 101;
    obj["statusString"] = "OK";
    obj["version"] = protocolVersion;
    obj["spotifyError"] = 0;
    obj["libraryVersion"] = swVersion;
    obj["accountReq"] = "PREMIUM";
    obj["brandDisplayName"] = brandName;
    obj["modelDisplayName"] = name.c_str();
    obj["voiceSupport"] = "NO";
    obj["availability"] = "";
    obj["productID"] = 0;
    obj["tokenType"] = "default";
    obj["groupStatus"] = "NONE";
    obj["resolverVersion"] = "0";
    obj["scope"] = "streaming,client-authorization-universal";
    obj["activeUser"] = "";
    obj["deviceID"] = deviceId;
    obj["remoteName"] = name.c_str();
    obj["publicKey"] = encodedKey;
    obj["deviceType"] = "SPEAKER";
    return obj.toString();
}
