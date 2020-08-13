
#include "Connection.h"

Connection::Connection(std::string apAddress){};
void Connection::connect()
{
}

void Connection::recvPacket()
{
}

void Connection::handshakeCompleted(std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey)
{
}

void Connection::sendPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> data)
{
}

