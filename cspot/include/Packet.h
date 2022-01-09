#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <cstdint>

class Packet
{
private:

public:
    Packet(uint8_t command, const std::vector<uint8_t> &data);
    uint8_t command;
    std::vector<uint8_t> data;
};

#endif