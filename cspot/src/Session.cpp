#include "Session.h"
#include "MercuryManager.h"
#include "Logger.h"

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t>;

Session::Session()
{
    // Generates the public and priv key
    this->crypto = std::make_unique<Crypto>();
    this->shanConn = std::make_shared<ShannonConnection>();
}

void Session::connect(std::unique_ptr<PlainConnection> connection)
{
    this->conn = std::move(connection);
    auto helloPacket = this->sendClientHelloRequest();
    this->processAPHelloResponse(helloPacket);
}

void Session::connectWithRandomAp()
{
    auto apResolver = std::make_unique<ApResolve>();
    this->conn = std::make_unique<PlainConnection>();

    auto apAddr = apResolver->fetchFirstApAddress();
    CSPOT_LOG(debug, "Connecting with AP <%s>", apAddr.c_str());
    this->conn->connectToAp(apAddr);
    auto helloPacket = this->sendClientHelloRequest();
    CSPOT_LOG(debug, "Sending APHello packet...");
    this->processAPHelloResponse(helloPacket);
}

std::vector<uint8_t> Session::authenticate(std::shared_ptr<LoginBlob> blob)
{
    // save auth blob for reconnection purposes
    authBlob = blob;

    // prepare authentication request proto
    authRequest.login_credentials.username = blob->username;
    authRequest.login_credentials.auth_data = blob->authData;
    authRequest.login_credentials.typ = static_cast<AuthenticationType>(blob->authType);
    authRequest.system_info.cpu_family = CpuFamily::CPU_UNKNOWN;
    authRequest.system_info.os = Os::OS_UNKNOWN;
    authRequest.system_info.system_information_string = std::string(informationString);
    authRequest.system_info.device_id = std::string(deviceId);
    authRequest.version_string = std::string(versionString);

    auto data = encodePb(authRequest);

    // Send login request
    this->shanConn->sendPacket(LOGIN_REQUEST_COMMAND, data);

    auto packet = this->shanConn->recvPacket();
    switch (packet->command)
    {
    case AUTH_SUCCESSFUL_COMMAND:
    {
        CSPOT_LOG(debug, "Authorization successful");

        // @TODO store the reusable credentials
        // PBWrapper<APWelcome> welcomePacket(packet->data)
        return std::vector<uint8_t>({0x1}); // TODO: return actual reusable credentaials to be stored somewhere
        break;
    }
    case AUTH_DECLINED_COMMAND:
    {
        CSPOT_LOG(error, "Authorization declined");
        break;
    }
    default:
        CSPOT_LOG(error, "Unknown auth fail code %d", packet->command);
    }

    return std::vector<uint8_t>(0);
}

void Session::processAPHelloResponse(std::vector<uint8_t> &helloPacket)
{
    CSPOT_LOG(debug, "Processing AP hello response...");
    auto data = this->conn->recvPacket();
    CSPOT_LOG(debug, "Received AP hello response");
    // Decode the response
    auto skipSize = std::vector<uint8_t>(data.begin() + 4, data.end());
    apResponse = decodePb<APResponseMessage>(skipSize);

    auto kkEy = apResponse.challenge->login_crypto_challenge.diffie_hellman->gs;
    // Compute the diffie hellman shared key based on the response
    auto sharedKey = this->crypto->dhCalculateShared(kkEy);

    // Init client packet + Init server packets are required for the hmac challenge
    data.insert(data.begin(), helloPacket.begin(), helloPacket.end());

    // Solve the hmac challenge
    auto resultData = std::vector<uint8_t>(0);
    for (int x = 1; x < 6; x++)
    {
        auto challengeVector = std::vector<uint8_t>(1);
        challengeVector[0] = x;

        challengeVector.insert(challengeVector.begin(), data.begin(), data.end());
        auto digest = crypto->sha1HMAC(sharedKey, challengeVector);
        resultData.insert(resultData.end(), digest.begin(), digest.end());
    }

    auto lastVec = std::vector<uint8_t>(resultData.begin(), resultData.begin() + 0x14);

    // Digest generated!
    clientResPlaintext.login_crypto_response = {};
    clientResPlaintext.login_crypto_response.diffie_hellman.emplace();
    clientResPlaintext.login_crypto_response.diffie_hellman->hmac = crypto->sha1HMAC(lastVec, data);

    auto resultPacket = encodePb(clientResPlaintext);

    auto emptyPrefix = std::vector<uint8_t>(0);

    this->conn->sendPrefixPacket(emptyPrefix, resultPacket);

    // Get send and receive keys
    auto sendKey = std::vector<uint8_t>(resultData.begin() + 0x14, resultData.begin() + 0x34);
    auto recvKey = std::vector<uint8_t>(resultData.begin() + 0x34, resultData.begin() + 0x54);

    CSPOT_LOG(debug, "Received shannon keys");

    // Init shanno-encrypted connection
    this->shanConn->wrapConnection(this->conn, sendKey, recvKey);
}

void Session::close() {
    this->conn->closeSocket();
}

std::vector<uint8_t> Session::sendClientHelloRequest()
{
    // Prepare protobuf message
    this->crypto->dhInit();

    // Copy the public key into diffiehellman hello packet
    clientHello.login_crypto_hello.diffie_hellman.emplace();
    clientHello.feature_set.emplace();
    clientHello.login_crypto_hello.diffie_hellman->gc = this->crypto->publicKey;
    clientHello.login_crypto_hello.diffie_hellman->server_keys_known = 1;
    clientHello.build_info.product = Product::PRODUCT_PARTNER;
    clientHello.build_info.platform = Platform::PLATFORM_LINUX_X86;
    clientHello.build_info.version = 112800721;
    clientHello.feature_set->autoupdate2 = true;
    clientHello.cryptosuites_supported = std::vector<Cryptosuite>({Cryptosuite::CRYPTO_SUITE_SHANNON});
    clientHello.padding = std::vector<uint8_t>({0x1E});

    // Generate the random nonce
    clientHello.client_nonce = crypto->generateVectorWithRandomData(16);
    auto vecData = encodePb(clientHello);
    auto prefix = std::vector<uint8_t>({0x00, 0x04});
    return this->conn->sendPrefixPacket(prefix, vecData);
}
