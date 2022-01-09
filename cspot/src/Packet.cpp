#include "Packet.h"

Packet::Packet(uint8_t command, const std::vector<uint8_t> &data) {
    this->command = command;
    this->data = data;
};