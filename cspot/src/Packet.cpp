#include "Packet.h"

Packet::Packet(uint8_t command, std::vector<uint8_t> &data) {
    this->command = command;
    this->data = data;
};