#ifndef SESSION_H
#define SESSION_H

#include "Packet.h"
#include "PlainConnection.h"
#include "ShannonConnection.h"
#include "DiffieHellman.h"
#include <vector>
#include "keyexchange.pb.h"
#include "authentication.pb.h"
#include <pb_encode.h>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include "PBUtils.h"
#include "Utils.h"

#define SPOTIFY_VERSION 0x10800000000
#define LOGIN_REQUEST_COMMAND 0xAB
#define AUTH_SUCCESSFUL_COMMAND 0xAC
#define AUTH_DECLINED_COMMAND 0xAD
#define DEVICE_ID "352198fd329622137e14901634264e6f332e5422"
#define INFORMATION_STRING "cspot"
#define VERSION_STRING "cspot-0.1"

class Session
{
private:
    PlainConnection *conn;
    ShannonConnection *shanConn;
    DiffieHellman *localKeys;
    std::vector<uint8_t> sendClientHelloRequest();
    void processAPHelloResponse(std::vector<uint8_t> &helloPacket);
public:
    Session();
    void connect(PlainConnection *connection);
    void authenticate(std::string login, std::string password);
};

#endif