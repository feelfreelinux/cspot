#pragma once


#include <algorithm>
#include <climits>
#include <functional>
#include <memory>
#include <random>
#include <vector>

#include "Crypto.h"
#include "Logger.h"
#include "NanoPBHelper.h"
#include "Utils.h"

#include "protobuf/authentication.pb.h"
#include "protobuf/keyexchange.pb.h"

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