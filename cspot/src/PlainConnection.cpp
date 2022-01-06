
#include "PlainConnection.h"
#include <cstring>
#include <netinet/tcp.h>
#include <errno.h>
#include "Logger.h"

PlainConnection::PlainConnection()
{
	this->apSock = -1;
};

PlainConnection::~PlainConnection()
{
    closeSocket();
};

void PlainConnection::connectToAp(std::string apAddress)
{
    struct addrinfo h, *airoot, *ai;
    std::string hostname = apAddress.substr(0, apAddress.find(":"));
    std::string portStr = apAddress.substr(apAddress.find(":") + 1, apAddress.size());
    memset(&h, 0, sizeof(h));
    h.ai_family = AF_INET;
    h.ai_socktype = SOCK_STREAM;
    h.ai_protocol = IPPROTO_IP;

    // Lookup host
    if (getaddrinfo(hostname.c_str(), portStr.c_str(), &h, &airoot))
    {
        CSPOT_LOG(error, "getaddrinfo failed");
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
        {
            struct timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;
            setsockopt(this->apSock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
            setsockopt(this->apSock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof tv);

            int flag = 1;
            setsockopt(this->apSock,  /* socket affected */
                       IPPROTO_TCP,   /* set option at TCP level */
                       TCP_NODELAY,   /* name of option */
                       (char *)&flag, /* the cast is historical cruft */
                       sizeof(int));  /* length of option value */
            break;
        }

        close(this->apSock);
        apSock = -1;
        throw std::runtime_error("Can't connect to spotify servers");
    }

    freeaddrinfo(airoot);
    CSPOT_LOG(debug, "Connected to spotify server");
}

std::vector<uint8_t> PlainConnection::recvPacket()
{
    // Read packet size
    auto sizeData = readBlock(4);
    uint32_t packetSize = ntohl(extract<uint32_t>(sizeData, 0));

    // Read actual data
    auto data = readBlock(packetSize - 4);
    sizeData.insert(sizeData.end(), data.begin(), data.end());

    return sizeData;
}

std::vector<uint8_t> PlainConnection::sendPrefixPacket(const std::vector<uint8_t> &prefix, const std::vector<uint8_t> &data)
{
    // Calculate full packet length
    uint32_t actualSize = prefix.size() + data.size() + sizeof(uint32_t);

    // Packet structure [PREFIX] + [SIZE] +  [DATA]
    auto sizeRaw = pack<uint32_t>(htonl(actualSize));
    sizeRaw.insert(sizeRaw.begin(), prefix.begin(), prefix.end());
    sizeRaw.insert(sizeRaw.end(), data.begin(), data.end());

    // Actually write it to the server
    writeBlock(sizeRaw);

    return sizeRaw;
}

std::vector<uint8_t> PlainConnection::readBlock(size_t size)
{
    std::vector<uint8_t> buf(size);
    unsigned int idx = 0;
    ssize_t n;
    int retries = 0;
    // printf("START READ\n");

    while (idx < size)
    {
    READ:
        if ((n = recv(this->apSock, &buf[idx], size - idx, 0)) <= 0)
        {
            switch (errno)
            {
            case EAGAIN:
            case ETIMEDOUT:
                if (timeoutHandler())
                {
                    CSPOT_LOG(error, "Connection lost, will need to reconnect...");
                    throw std::runtime_error("Reconnection required");
                }
                goto READ;
            case EINTR:
                break;
            default:
                if (retries++ > 4) throw std::runtime_error("Error in read");

            }
        }
        idx += n;
    }
    // printf("FINISH READ\n");
    return buf;
}

size_t PlainConnection::writeBlock(const std::vector<uint8_t> &data)
{
    unsigned int idx = 0;
    ssize_t n;
    // printf("START WRITE\n");
    int retries = 0;

    while (idx < data.size())
    {
    WRITE:
        if ((n = send(this->apSock, &data[idx], data.size() - idx < 64 ? data.size() - idx : 64, 0)) <= 0)
        {
            switch (errno)
            {
            case EAGAIN:
            case ETIMEDOUT:
                if (timeoutHandler())
                {
                    throw std::runtime_error("Reconnection required");
                }
                goto WRITE;
            case EINTR:
                break;
            default:
                if (retries++ > 4) throw std::runtime_error("Error in write");
            }
        }
        idx += n;
    }

    return data.size();
}

void PlainConnection::closeSocket()
{
	if (this->apSock < 0) return;

	CSPOT_LOG(info, "Closing socket...");
	shutdown(this->apSock, SHUT_RDWR);
	close(this->apSock);
	this->apSock = -1;
}
