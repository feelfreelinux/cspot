#ifndef PLAINCONNECTION_H
#define PLAINCONNECTION_H
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "win32shim.h"
#else
#include "sys/socket.h"
#include <netdb.h>
#include <unistd.h>
#endif
#include <functional>
#include <vector>
#include <string>
#include <cstdint>
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