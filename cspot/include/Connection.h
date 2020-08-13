#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <vector>
#include <string>
#include <cstdint>

enum class ConnectType
{
    Handshake,
    Stream
};

class Connection
{
private:
    int apSock;
    std::vector<uint8_t> partialBuffer;
    ConnectType connectType;

public:
    Connection(std::string apAddress);
    void connect();
    void recvPacket();
    void handshakeCompleted(std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey);
    void sendPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> data);
};

#endif