#ifndef PACKET_H
#define PACKET_H

#include <vector>

class Packet
{
private:

public:
    Packet(int command, size_t packetSize, std::vector<uint8_t> data);
    int command;
    size_t packetSize;
    std::vector<uint8_t> data;
};

#endif