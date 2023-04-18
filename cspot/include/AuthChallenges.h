#pragma once


#include <cstdint>                       // for uint8_t
#include <memory>                        // for unique_ptr
#include <string>                        // for string
#include <vector>                        // for vector

#include "Crypto.h"                      // for Crypto
#include "protobuf/authentication.pb.h"  // for ClientResponseEncrypted
#include "protobuf/keyexchange.pb.h"     // for APResponseMessage, ClientHello

namespace cspot {
class AuthChallenges {
 public:
  AuthChallenges();
  ~AuthChallenges();

  std::vector<uint8_t> shanSendKey = {};
  std::vector<uint8_t> shanRecvKey = {};

  std::vector<uint8_t> prepareAuthPacket(std::vector<uint8_t>& authBlob,
                                         int authType,
                                         const std::string& deviceId,
                                         const std::string& username);
  std::vector<uint8_t> solveApHello(std::vector<uint8_t>& helloPacket,
                                    std::vector<uint8_t>& data);

  std::vector<uint8_t> prepareClientHello();

 private:
  const long long SPOTIFY_VERSION = 0x10800000000;
  ClientResponseEncrypted authRequest;
  ClientResponsePlaintext clientResPlaintext;
  ClientHello clientHello;
  APResponseMessage apResponse;

  std::unique_ptr<Crypto> crypto;
};
}  // namespace cspot