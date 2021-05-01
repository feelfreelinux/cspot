#include "ShannonConnection.h"
#include "Logger.h"

ShannonConnection::ShannonConnection()
{
}

ShannonConnection::~ShannonConnection()
{
}

void ShannonConnection::wrapConnection(std::shared_ptr<PlainConnection> conn, std::vector<uint8_t> &sendKey, std::vector<uint8_t> &recvKey)
{
    this->conn = conn;

    this->sendCipher = std::make_unique<Shannon>();
    this->recvCipher = std::make_unique<Shannon>();

    // Set keys
    this->sendCipher->key(sendKey);
    this->recvCipher->key(recvKey);

    // Set initial nonce
    this->sendCipher->nonce(pack<uint32_t>(htonl(0)));
    this->recvCipher->nonce(pack<uint32_t>(htonl(0)));
}

void ShannonConnection::sendPacket(uint8_t cmd, std::vector<uint8_t> &data)
{
    this->writeMutex.lock();
    auto rawPacket = this->cipherPacket(cmd, data);

    // Shannon encrypt the packet and write it to sock
    this->sendCipher->encrypt(rawPacket);
    this->conn->writeBlock(rawPacket);

    // Generate mac
    std::vector<uint8_t> mac(MAC_SIZE);
    this->sendCipher->finish(mac);

    // Update the nonce
    this->sendNonce += 1;
    this->sendCipher->nonce(pack<uint32_t>(htonl(this->sendNonce)));

    // Write the mac to sock
    this->conn->writeBlock(mac);
    this->writeMutex.unlock();
}

std::unique_ptr<Packet> ShannonConnection::recvPacket()
{
    this->readMutex.lock();
    // Receive 3 bytes, cmd + int16 size
    auto data = this->conn->readBlock(3);
    this->recvCipher->decrypt(data);

    auto packetData = std::vector<uint8_t>();

    auto readSize = ntohs(extract<uint16_t>(data, 1));

    // Read and decode if the packet has an actual body
    if (readSize > 0)
    {
        packetData = this->conn->readBlock(readSize);
        this->recvCipher->decrypt(packetData);
    }

    // Read mac
    auto mac = this->conn->readBlock(MAC_SIZE);

    // Generate mac
    std::vector<uint8_t> mac2(MAC_SIZE);
    this->recvCipher->finish(mac2);

    if (mac != mac2)
    {
        CSPOT_LOG(error, "Shannon read: Mac doesn't match");
    }

    // Update the nonce
    this->recvNonce += 1;
    this->recvCipher->nonce(pack<uint32_t>(htonl(this->recvNonce)));

    // Unlock the mutex
    this->readMutex.unlock();

    // data[0] == cmd
    return std::make_unique<Packet>(data[0], packetData);
}

std::vector<uint8_t> ShannonConnection::cipherPacket(uint8_t cmd, std::vector<uint8_t> &data)
{
    // Generate packet structure, [Command] [Size] [Raw data]
    auto sizeRaw = pack<uint16_t>(htons(uint16_t(data.size())));

    sizeRaw.insert(sizeRaw.begin(), cmd);
    sizeRaw.insert(sizeRaw.end(), data.begin(), data.end());

    return sizeRaw;
}
