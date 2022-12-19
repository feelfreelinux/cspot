#pragma once

#include <memory>

#include "MercurySession.h"
#include "TimeProvider.h"

namespace cspot {
struct Context {
  struct ConfigState {
    std::string deviceId;
    std::string deviceName;
    int volume;

    std::string username;
    std::string countryCode;
  };

  ConfigState config;

  std::shared_ptr<TimeProvider> timeProvider;
  std::shared_ptr<cspot::MercurySession> session;

  static std::shared_ptr<Context> createFromBlob(std::shared_ptr<LoginBlob> blob) {
    auto ctx = std::make_shared<Context>();
    ctx->timeProvider = std::make_shared<TimeProvider>();

    ctx->session = std::make_shared<MercurySession>(ctx->timeProvider);
    ctx->config = {
        .deviceId = blob->getDeviceId(),
        .deviceName = "cspot",
        .volume = 0,
        .username = blob->getUserName()
    };

    return ctx;
  }

};
}  // namespace cspot