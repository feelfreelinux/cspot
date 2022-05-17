#include "Session.h"
#include "MercuryManager.h"
#include "Logger.h"

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t>;

Session::Session()
{
    this->clientHello = {};
    this->apResponse = {};
    this->authRequest = {};
    this->clientResPlaintext = {};

    // Generates the public and priv key
    this->crypto = std::make_unique<Crypto>();
    this->shanConn = std::make_shared<ShannonConnection>();
}

Session::~Session()
{
    pb_release(ClientHello_fields, &clientHello);
    pb_release(APResponseMessage_fields, &apResponse);
    pb_release(ClientResponsePlaintext_fields, &clientResPlaintext);
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
    pbPutString(blob->username, authRequest.login_credentials.username);

    std::copy(blob->authData.begin(), blob->authData.end(), authRequest.login_credentials.auth_data.bytes);
    authRequest.login_credentials.auth_data.size = blob->authData.size();

    authRequest.login_credentials.typ = (AuthenticationType) blob->authType;
    authRequest.system_info.cpu_family = CpuFamily_CPU_UNKNOWN;
    authRequest.system_info.os = Os_OS_UNKNOWN;

    auto infoStr = std::string(informationString);
    pbPutString(infoStr, authRequest.system_info.system_information_string);

    auto deviceIdStr = std::string(deviceId);
    pbPutString(deviceId, authRequest.system_info.device_id);

    auto versionStr = std::string(versionString);
    pbPutString(versionStr, authRequest.version_string);
    authRequest.has_version_string = true;

    auto data = pbEncode(ClientResponseEncrypted_fields, &authRequest);

    // Send login request
    this->shanConn->sendPacket(LOGIN_REQUEST_COMMAND, data);

    auto packet = this->shanConn->recvPacket();
    switch (packet->command)
    {
    case AUTH_SUCCESSFUL_COMMAND:
    {
        CSPOT_LOG(debug, "Authorization successful");

        // @TODO store the reusable credentials
        // PBWrapper<APWelcome>  welcomePacket(packet->data)
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

    pb_release(APResponseMessage_fields, &apResponse);
    pbDecode(apResponse, APResponseMessage_fields, skipSize);

    auto diffieKey = std::vector<uint8_t>(apResponse.challenge.login_crypto_challenge.diffie_hellman.gs, apResponse.challenge.login_crypto_challenge.diffie_hellman.gs + 96);
    // Compute the diffie hellman shared key based on the response
    auto sharedKey = this->crypto->dhCalculateShared(diffieKey);

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
    auto digest = crypto->sha1HMAC(lastVec, data);
    clientResPlaintext.login_crypto_response.has_diffie_hellman = true;

    std::copy(digest.begin(),
              digest.end(),
              clientResPlaintext.login_crypto_response.diffie_hellman.hmac);

    auto resultPacket = pbEncode(ClientResponsePlaintext_fields, &clientResPlaintext);

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
    std::copy(this->crypto->publicKey.begin(),
              this->crypto->publicKey.end(),
              clientHello.login_crypto_hello.diffie_hellman.gc);

    clientHello.login_crypto_hello.diffie_hellman.server_keys_known = 1;
    clientHello.build_info.product = Product_PRODUCT_CLIENT;
    clientHello.build_info.platform = Platform2_PLATFORM_LINUX_X86;
    clientHello.build_info.version = SPOTIFY_VERSION;
    clientHello.feature_set.autoupdate2 = true;
    clientHello.cryptosuites_supported[0] = Cryptosuite_CRYPTO_SUITE_SHANNON;
    clientHello.padding[0] = 0x1E;

    clientHello.has_feature_set = true;
    clientHello.login_crypto_hello.has_diffie_hellman = true;
    clientHello.has_padding = true;
    clientHello.has_feature_set = true;

    // Generate the random nonce
    auto nonce = crypto->generateVectorWithRandomData(16);
    std::copy(nonce.begin(), nonce.end(), clientHello.client_nonce);
    auto vecData = pbEncode(ClientHello_fields, &clientHello);
    auto prefix = std::vector<uint8_t>({0x00, 0x04});
    return this->conn->sendPrefixPacket(prefix, vecData);
}
