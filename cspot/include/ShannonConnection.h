#ifndef SHANNONCONNECTION_H
#define SHANNONCONNECTION_H

#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <netdb.h>
#include <memory>
#include <cstdint>
#include "platform/WrappedMutex.h"
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
    WrappedMutex writeMutex;
    WrappedMutex readMutex;
    
public:
    ShannonConnection();
    ~ShannonConnection();
    void wrapConnection(std::shared_ptr<PlainConnection> conn, std::vector<uint8_t> &sendKey, std::vector<uint8_t> &recvKey);
    void sendPacket(uint8_t cmd, std::vector<uint8_t> &data);
    std::shared_ptr<PlainConnection> conn;
    std::unique_ptr<Packet> recvPacket();
};

#endif
