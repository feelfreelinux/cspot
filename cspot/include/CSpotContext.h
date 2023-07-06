#pragma once

#include <_types/_uint8_t.h>
#include <memory>

#include "LoginBlob.h"
#include "MercurySession.h"
#include "TimeProvider.h"
#include "protobuf/metadata.pb.h"

namespace cspot {
struct Context {
  struct ConfigState {
    // Setup default bitrate to 160
    AudioFormat audioFormat = AudioFormat::AudioFormat_OGG_VORBIS_160;
    std::string deviceId;
    std::string deviceName;
    std::vector<uint8_t> authData;
    int volume;

    std::string username;
    std::string countryCode;
  };

  ConfigState config;

  std::shared_ptr<TimeProvider> timeProvider;
  std::shared_ptr<cspot::MercurySession> session;

  static std::shared_ptr<Context> createFromBlob(
      std::shared_ptr<LoginBlob> blob) {
    auto ctx = std::make_shared<Context>();
    ctx->timeProvider = std::make_shared<TimeProvider>();

    ctx->session = std::make_shared<MercurySession>(ctx->timeProvider);
    ctx->config.deviceId = blob->getDeviceId();
    ctx->config.deviceName = blob->getDeviceName();
    ctx->config.authData = blob->authData;
    ctx->config.volume = 0;
    ctx->config.username = blob->getUserName();

    return ctx;
  }
};
}  // namespace cspot
