#include "Packet.h"

Packet::Packet(int command, size_t packetSize, std::vector<uint8_t> data){
    this->command = command;
    this->packetSize = packetSize;
    this->data = data;
};