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

class PlainConnection
{
public:
    PlainConnection();
    int apSock;
    void connectToAp(std::string apAddress);
    std::vector<uint8_t> sendPrefixPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> &data);
    std::vector<uint8_t> recvPacket();
};

#endif