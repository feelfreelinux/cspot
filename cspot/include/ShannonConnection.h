#ifndef SHANNONCONNECTION_H
#define SHANNONCONNECTION_H

#include <sys/types.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Packet.h"
#include "PlainConnection.h"
#include "Shannon.h"
#include "Utils.h"
#include "Logger.h"

#define MAC_SIZE 4
namespace cspot {
class ShannonConnection {
 private:
  std::unique_ptr<Shannon> sendCipher;
  std::unique_ptr<Shannon> recvCipher;
  uint32_t sendNonce = 0;
  uint32_t recvNonce = 0;
  std::vector<uint8_t> cipherPacket(uint8_t cmd, std::vector<uint8_t>& data);
  std::mutex writeMutex;
  std::mutex readMutex;

 public:
  ShannonConnection();
  ~ShannonConnection();
  void wrapConnection(std::shared_ptr<PlainConnection> conn,
                      std::vector<uint8_t>& sendKey,
                      std::vector<uint8_t>& recvKey);
  void sendPacket(uint8_t cmd, std::vector<uint8_t>& data);
  std::shared_ptr<PlainConnection> conn;
  Packet recvPacket();
};
}  // namespace cspot

#endif
