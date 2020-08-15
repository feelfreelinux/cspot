#ifndef SESSION_H
#define SESSION_H

#include "Packet.h"
#include "PlainConnection.h"
#include "ShannonConnection.h"
#include "DiffieHellman.h"
#include <vector>
#include <keyexchange.pb.h>
#include <pb_encode.h>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include "PBUtils.h"
#include "Utils.h"

#define SPOTIFY_VERSION 0x10800000000
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
};

#endif