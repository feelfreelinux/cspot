#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <cstdint>
#include <netdb.h>
#include <unistd.h>
#include <Packet.h>

// @TODO: actually get these through apresolve.spotify.com
#define AP_ADDRES "gew1-accesspoint-b-sk8w.ap.spotify.com"
#define AP_PORT "4070"

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
    std::vector<uint8_t> tryRecv(size_t size);

public:
    Connection();
    void connectToAp();
    Packet recvPacket();
    std::vector<uint8_t> sendPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> data);
    void handshakeCompleted(std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey);
};

#endif