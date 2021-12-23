#ifndef PLAINCONNECTION_H
#define PLAINCONNECTION_H

#include "sys/socket.h"
#include <functional>
#include <vector>
#include <string>
#include <cstdint>
#include <netdb.h>
#include <unistd.h>
#include "Packet.h"
#include "Utils.h"

typedef std::function<bool()> timeoutCallback;

class PlainConnection
{
public:
    PlainConnection();
    ~PlainConnection();
    int apSock;
    void connectToAp(std::string apAddress);
    void closeSocket();
    timeoutCallback timeoutHandler;
    std::vector<uint8_t> sendPrefixPacket(const std::vector<uint8_t> &prefix, const std::vector<uint8_t> &data);
    std::vector<uint8_t> recvPacket();
    std::vector<uint8_t> readBlock(size_t size);
    size_t writeBlock(const std::vector<uint8_t> &data);
};

#endif