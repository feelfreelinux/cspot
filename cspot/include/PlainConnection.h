#ifndef PLAINCONNECTION_H
#define PLAINCONNECTION_H

#include "sys/socket.h"
#include <vector>
#include <string>
#include <cstdint>
#include <netdb.h>
#include <unistd.h>
#include "Packet.h"
#include "Utils.h"

// @TODO: actually get these through apresolve.spotify.com
#define AP_ADDRESS "gae2-accesspoint-b-3px2.ap.spotify.com"
#define AP_PORT "4070"

class PlainConnection
{
public:
    PlainConnection();
    int apSock;
    void connectToAp();
    std::vector<uint8_t> sendPrefixPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> &data);
    std::vector<uint8_t> recvPacket();
};

#endif