#ifndef SHANNONCONNECTION_H
#define SHANNONCONNECTION_H

#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "win32shim.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <mutex>
#include <netdb.h>
#endif
#include <sys/types.h>
#include <string>
#include <memory>
#include <cstdint>
#include "Utils.h"
#include "Shannon.h"
#include "PlainConnection.h"
#include "Packet.h"

#define MAC_SIZE 4


class ShannonConnection
{
private:
    std::unique_ptr<Shannon> sendCipher;
    std::unique_ptr<Shannon> recvCipher;
    uint32_t sendNonce = 0;
    uint32_t recvNonce = 0;
    std::vector<uint8_t> cipherPacket(uint8_t cmd, std::vector<uint8_t> &data);
    std::mutex writeMutex;
    std::mutex readMutex;
    
public:
    ShannonConnection();
    ~ShannonConnection();
    void wrapConnection(std::shared_ptr<PlainConnection> conn, std::vector<uint8_t> &sendKey, std::vector<uint8_t> &recvKey);
    void sendPacket(uint8_t cmd, std::vector<uint8_t> &data);
    std::shared_ptr<PlainConnection> conn;
    std::unique_ptr<Packet> recvPacket();
};

#endif
