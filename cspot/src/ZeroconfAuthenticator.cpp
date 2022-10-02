#include "ZeroconfAuthenticator.h"
#include "JSONObject.h"
#include <sstream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Logger.h"
#include "ConfigJSON.h"

// provide weak deviceId (see ConstantParameters.h)
char deviceId[] __attribute__((weak)) = "142137fd329622137a14901634264e6f332e2411";

ZeroconfAuthenticator::ZeroconfAuthenticator(authCallback callback, std::shared_ptr<bell::BaseHTTPServer> httpServer) {
    this->gotBlobCallback = callback;
    srand((unsigned int)time(NULL));

    this->crypto = std::make_unique<Crypto>();
    this->crypto->dhInit();
    this->server = httpServer;
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
    const char* service = "_spotify-connect._tcp";

#ifdef ESP_PLATFORM
    mdns_txt_item_t serviceTxtData[3] = {
        {"VERSION", "1.0"},
        {"CPath", "/spotify_info"},
        {"Stack", "SP"} };
    mdns_service_add("cspot", "_spotify-connect", "_tcp", this->server->serverPort, serviceTxtData, 3);

#else
    DNSServiceRef ref = NULL;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);
    TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    TXTRecordSetValue(&txtRecord, "CPath", 13, "/spotify_info");
    TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    DNSServiceRegister(&ref, 0, 0, (char*)informationString, service, NULL, NULL, htons(this->server->serverPort), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
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
    obj["modelDisplayName"] = configMan->deviceName.c_str();
    obj["voiceSupport"] = "NO";
    obj["availability"] = "";
    obj["productID"] = 0;
    obj["tokenType"] = "default";
    obj["groupStatus"] = "NONE";
    obj["resolverVersion"] = "0";
    obj["scope"] = "streaming,client-authorization-universal";
    obj["activeUser"] = "";
    obj["deviceID"] = deviceId;
    obj["remoteName"] = configMan->deviceName.c_str();
    obj["publicKey"] = encodedKey;
    obj["deviceType"] = "SPEAKER";
    return obj.toString();
}
