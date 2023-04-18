#pragma once

#include <functional>  // for function
#include <memory>      // for shared_ptr
#include <string>      // for string

namespace cspot {
struct Context;

class AccessKeyFetcher {
 public:
  AccessKeyFetcher(std::shared_ptr<cspot::Context> ctx);
  ~AccessKeyFetcher();

  typedef std::function<void(std::string)> Callback;

  void getAccessKey(Callback callback);

 private:
  const std::string CLIENT_ID =
      "65b708073fc0480ea92a077233ca87bd";  // Spotify web client's client id
  const std::string SCOPES =
      "streaming,user-library-read,user-library-modify,user-top-read,user-read-"
      "recently-played";  // Required access scopes

  std::shared_ptr<cspot::Context> ctx;

  bool isExpired();
  std::string accessKey;
  long long int expiresAt;
};
}  // namespace cspot
