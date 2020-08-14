#ifndef SHANNONCONNECTION_H
#define SHANNONCONNECTION_H

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <cstdint>
#include <netdb.h>
#include <unistd.h>
#include "Packet.h"
#include "Utils.h"
#include "PlainConnection.h"
#include "Shannon.h"

#define MAC_SIZE 4

class ShannonConnection
{
private:
    Shannon *sendCipher;
    Shannon *recvCipher;
    uint32_t sendNonce = 0;
    uint32_t recvNonce = 0;
    std::vector<uint8_t> cipherPacket(uint8_t cmd, std::vector<uint8_t> data);
public:
    ShannonConnection(PlainConnection conn, std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey);
    int apSock;
    void sendPacket(uint8_t cmd, std::vector<uint8_t> data);
    Packet* recvPacket();
};

#endif