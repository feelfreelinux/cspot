#include "Session.h"
#include <memory>
#include "AuthChallenges.h"

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t>;

using namespace cspot;

Session::Session() 
{
    this->challenges = std::make_unique<cspot::AuthChallenges>();

    // Generates the public and priv key
    this->shanConn = std::make_shared<ShannonConnection>();
}

Session::~Session()
{
}

void Session::connect(std::unique_ptr<cspot::PlainConnection> connection)
{
    this->conn = std::move(connection);
    auto helloPacket = this->conn->sendPrefixPacket({0x00, 0x04}, this->challenges->prepareClientHello());
    auto apResponse = this->conn->recvPacket();
    CSPOT_LOG(info, "Received APHello response");

    auto solvedHello = this->challenges->solveApHello(helloPacket, apResponse);

    conn->sendPrefixPacket({}, solvedHello);
    CSPOT_LOG(debug, "Received shannon keys");

    // Init shanno-encrypted connection
    this->shanConn->wrapConnection(this->conn, challenges->shanSendKey, challenges->shanRecvKey);
}

void Session::connectWithRandomAp()
{
    auto apResolver = std::make_unique<ApResolve>("");
    auto conn = std::make_unique<cspot::PlainConnection>();

    auto apAddr = apResolver->fetchFirstApAddress();

    CSPOT_LOG(debug, "Connecting with AP <%s>", apAddr.c_str());
    conn->connect(apAddr);

    this->connect(std::move(conn));
}

std::vector<uint8_t> Session::authenticate(std::shared_ptr<LoginBlob> blob)
{
    // save auth blob for reconnection purposes
    authBlob = blob;
    // prepare authentication request proto
    auto data = challenges->prepareAuthPacket(blob->authData, blob->authType, deviceId, blob->username);

    // Send login request
    this->shanConn->sendPacket(LOGIN_REQUEST_COMMAND, data);

    auto packet = this->shanConn->recvPacket();
    switch (packet.command)
    {
    case AUTH_SUCCESSFUL_COMMAND:
    {
        CSPOT_LOG(debug, "Authorization successful");
        return std::vector<uint8_t>({0x1}); // TODO: return actual reusable credentaials to be stored somewhere
        break;
    }
    case AUTH_DECLINED_COMMAND:
    {
        CSPOT_LOG(error, "Authorization declined");
        break;
    }
    default:
        CSPOT_LOG(error, "Unknown auth fail code %d", packet.command);
    }

    return std::vector<uint8_t>(0);
}

void Session::close() {
    this->conn->close();
}