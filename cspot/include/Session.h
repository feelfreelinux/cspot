#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "ApResolve.h"
#include "AuthChallenges.h"
#include "ConstantParameters.h"
#include "Logger.h"
#include "LoginBlob.h"
#include "Packet.h"
#include "PlainConnection.h"
#include "ShannonConnection.h"
#include "Utils.h"
#include "protobuf/mercury.pb.h"

#define LOGIN_REQUEST_COMMAND 0xAB
#define AUTH_SUCCESSFUL_COMMAND 0xAC
#define AUTH_DECLINED_COMMAND 0xAD

namespace cspot {
class Session {
 protected:
  std::unique_ptr<cspot::AuthChallenges> challenges;
  std::shared_ptr<cspot::PlainConnection> conn;
  std::shared_ptr<LoginBlob> authBlob;

  std::string deviceId = "142137fd329622137a14901634264e6f332e2411";

 public:
  Session();
  ~Session();

  std::shared_ptr<cspot::ShannonConnection> shanConn;

  void connect(std::unique_ptr<cspot::PlainConnection> connection);
  void connectWithRandomAp();
  void close();
  virtual bool triggerTimeout() = 0;
  std::vector<uint8_t> authenticate(std::shared_ptr<LoginBlob> blob);
};
}  // namespace cspot
