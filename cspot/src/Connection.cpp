
#include "Connection.h"

Connection::Connection()
{
};

void Connection::connectToAp()
{
    struct addrinfo h, *airoot, *ai;

    memset(&h, 0, sizeof(h));
    h.ai_family = PF_UNSPEC;
    h.ai_socktype = SOCK_STREAM;
    h.ai_protocol = IPPROTO_TCP;

    // Lookup host
    if (getaddrinfo(AP_ADDRES, AP_PORT, &h, &airoot))
    {
        printf("getaddrinfo failed\n");
    }

    // find the right ai, connect to server
    for (ai = airoot; ai; ai = ai->ai_next)
    {
        if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
            continue;

        this->apSock = socket(ai->ai_family,
                        ai->ai_socktype, ai->ai_protocol);
        if (this->apSock < 0)
            continue;

        if (connect(this->apSock,
                    (struct sockaddr *)ai->ai_addr,
                    ai->ai_addrlen) != -1)
            break;

        close(this->apSock);
        apSock = -1;
    }

    freeaddrinfo(airoot);
    printf("Connected to spotify server\n");
}

std::vector<uint8_t> Connection::tryRecv(size_t readSize) {
    // Copy so it doesn't mess with partial buffer
    // std::vector<uint8_t> result(readSize);
    // blockRead(this->apSock, &result, readSize);
    // this->partialBuffer = result;
    // return result;
}

Packet Connection::recvPacket()
{
    if (this->connectType == ConnectType::Handshake) {
        unsigned long packetSize;
        recv(this->apSock, &packetSize, sizeof(packetSize), 0);
        auto packetData = blockRead(this->apSock, packetSize);
        auto packet = new Packet(0, packetSize, packetData);

    }
}

void Connection::handshakeCompleted(std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey)
{
}

std::vector<uint8_t> Connection::sendPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> data)
{
}
