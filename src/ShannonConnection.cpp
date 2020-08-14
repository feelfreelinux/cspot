#include "ShannonConnection.h"

ShannonConnection::ShannonConnection(PlainConnection conn, std::vector<uint8_t> sendKey, std::vector<uint8_t> recvKey) {
}

void ShannonConnection::sendPacket(uint8_t cmd, std::vector<uint8_t> data) {

}

Packet ShannonConnection::recvPacket() {

}

void ShannonConnection::finishRecv() {
    
}

void ShannonConnection::finishSend() {

}

std::vector<uint8_t> ShannonConnection::cipherPacket(uint8_t cmd, std::vector<uint8_t> data) {

}
