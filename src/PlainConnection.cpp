
#include "PlainConnection.h"
#include <cstring>

PlainConnection::PlainConnection()
{
};

void PlainConnection::connectToAp()
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

std::vector<uint8_t> PlainConnection::recvPacket()
{
    // Read packet size
    auto sizeData = blockRead(this->apSock, 4);
    uint32_t packetSize = ntohl(extract<uint32_t>(sizeData, 0));

    // Read actual data
    auto data = blockRead(this->apSock, packetSize - 4);
    sizeData.insert(sizeData.end(), data.begin(), data.end());

    return sizeData;
}

std::vector<uint8_t> PlainConnection::sendPrefixPacket(std::vector<uint8_t> prefix, std::vector<uint8_t> &data)
{
    // Calculate full packet length
    uint32_t actualSize = prefix.size() + data.size() + sizeof(uint32_t);

    // Packet structure [PREFIX] + [SIZE] +  [DATA]
    auto sizeRaw = pack<uint32_t>(htonl(actualSize));
    sizeRaw.insert(sizeRaw.begin(), prefix.begin(), prefix.end());
    sizeRaw.insert(sizeRaw.end(), data.begin(), data.end());

    // Actually write it to the server
    blockWrite(this->apSock, sizeRaw);

    return sizeRaw;
}
