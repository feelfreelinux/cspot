#pragma once

#include <memory>
#include "CDNTrackStream.h"
#include "CSpotContext.h"
#include "TrackProvider.h"
#include "WrappedSemaphore.h"

namespace cspot {
class TrackPlayer: bell::Task {
 public:
  TrackPlayer(std::shared_ptr<cspot::Context> ctx);
  ~TrackPlayer();

  enum class Status {
    STOPPED,
    LOADING,
    PLAYING,
    PAUSED
  };
  Status playerStatus;

  void loadTrackFromRed(TrackRef* ref);

private:
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::TrackProvider> trackProvider;
  std::shared_ptr<cspot::CDNTrackStream> currentTrackStream;

  std::unique_ptr<bell::WrappedSemaphore> playbackSemaphore;

  void runTask() override;
};
}  // namespace cspot