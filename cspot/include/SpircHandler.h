#pragma once

#include <memory>
#include "BellTask.h"

#include "CSpotContext.h"
#include "PlaybackState.h"
#include "TrackPlayer.h"
#include "TrackProvider.h"
#include "protobuf/spirc.pb.h"

namespace cspot {
class SpircHandler {
 public:
  SpircHandler(std::shared_ptr<cspot::Context> ctx);

  void subscribeToMercury();

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::TrackPlayer> trackPlayer;

  cspot::PlaybackState playbackState;
  void sendCmd(MessageType typ);
  void handleFrame(std::vector<uint8_t> &data);
  void notify();
};
}  // namespace cspot