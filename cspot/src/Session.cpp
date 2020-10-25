#include "Session.h"
#include "MercuryManager.h"

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
    this->conn->connectToAp(apAddr);

    auto helloPacket = this->sendClientHelloRequest();
    this->processAPHelloResponse(helloPacket);
}

std::vector<uint8_t> Session::authenticate(std::shared_ptr<LoginBlob> blob)
{
    // save auth blob for reconnection purposes
    authBlob = blob;

    // prepare authentication request proto
    ClientResponseEncrypted authRequest = {};
    authRequest.login_credentials.username = (char *)(blob->username.c_str());
    authRequest.login_credentials.auth_data = vectorToPbArray(blob->authData);
    authRequest.login_credentials.typ = (AuthenticationType)blob->authType;
    authRequest.system_info.cpu_family = CpuFamily_CPU_UNKNOWN;
    authRequest.system_info.os = Os_OS_UNKNOWN;
    authRequest.system_info.system_information_string = (char *)informationString;
    authRequest.system_info.device_id = (char *)deviceId;
    authRequest.version_string = (char *)versionString;

    auto data = encodePB(ClientResponseEncrypted_fields, &authRequest);

    // Send login request
    this->shanConn->sendPacket(LOGIN_REQUEST_COMMAND, data);
    free(authRequest.login_credentials.auth_data);

    auto packet = this->shanConn->recvPacket();
    switch (packet->command)
    {
    case AUTH_SUCCESSFUL_COMMAND:
    {
        printf("Authorization successful\n");

        // @TODO store the reusable credentials
        // PBWrapper<APWelcome> welcomePacket(packet->data)
        return std::vector<uint8_t>({0x1}); // TODO: return actual reusable credentaials to be stored somewhere
        break;
    }
    case AUTH_DECLINED_COMMAND:
    {
        printf("Authorization declined\n");
        break;
    }
    default:
        printf("Unknown auth fail code %d\n", packet->command);
    }

    return std::vector<uint8_t>(0);
}

void Session::processAPHelloResponse(std::vector<uint8_t> &helloPacket)
{
    auto data = this->conn->recvPacket();

    // Decode the response
    auto skipSize = std::vector<uint8_t>(data.begin() + 4, data.end());
    printf("SKIPSIZE %d\n", skipSize.size());
    PBWrapper<APResponseMessage> res(skipSize);

    // Compute the diffie hellman shared key based on the response
    auto diffieKey = std::vector<uint8_t>(res->challenge.login_crypto_challenge.diffie_hellman.gs, res->challenge.login_crypto_challenge.diffie_hellman.gs + 96);
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

    ClientResponsePlaintext response = {};
    response.login_crypto_response.has_diffie_hellman = true;

    std::copy(digest.begin(),
              digest.end(),
              response.login_crypto_response.diffie_hellman.hmac);

    auto resultPacket = encodePB(ClientResponsePlaintext_fields, &response);
    auto emptyPrefix = std::vector<uint8_t>(0);

    this->conn->sendPrefixPacket(emptyPrefix, resultPacket);

    // Get send and receive keys
    auto sendKey = std::vector<uint8_t>(resultData.begin() + 0x14, resultData.begin() + 0x34);
    auto recvKey = std::vector<uint8_t>(resultData.begin() + 0x34, resultData.begin() + 0x54);

    // Init shanno-encrypted connection
    this->shanConn->wrapConnection(this->conn, sendKey, recvKey);
}

std::vector<uint8_t> Session::sendClientHelloRequest()
{
    // Prepare protobuf message
    ClientHello request = ClientHello_init_default;
    this->crypto->dhInit();

    // Copy the public key into diffiehellman hello packet
    std::copy(this->crypto->publicKey.begin(),
              this->crypto->publicKey.end(),
              request.login_crypto_hello.diffie_hellman.gc);

    request.login_crypto_hello.diffie_hellman.server_keys_known = 1;
    request.build_info.product = Product_PRODUCT_PARTNER;
    request.build_info.platform = Platform_PLATFORM_LINUX_X86;
    request.build_info.version = SPOTIFY_VERSION;
    request.feature_set.autoupdate2 = true;
    request.cryptosuites_supported[0] = Cryptosuite_CRYPTO_SUITE_SHANNON;
    request.padding[0] = 0x1E;

    request.has_feature_set = true;
    request.login_crypto_hello.has_diffie_hellman = true;
    request.has_padding = true;
    request.has_feature_set = true;

    // Generate the random nonce
    random_bytes_engine rbe;
    std::vector<uint8_t> nonce(16);
    std::generate(begin(nonce), end(nonce), std::ref(rbe));
    std::copy(nonce.begin(), nonce.end(), request.client_nonce);

    auto vecData = encodePB(ClientHello_fields, &request);

    auto prefix = std::vector<uint8_t>({0x00, 0x04});
    return this->conn->sendPrefixPacket(prefix, vecData);
}
