#pragma once

#include <stddef.h>  // for size_t
#include <memory>    // for shared_ptr
#include <vector>    // for vector

#include "protobuf/spirc.pb.h"  // for TrackRef

namespace cspot {
struct Context;
struct TrackReference;

class QueuedTrack {
 public:
  QueuedTrack(std::shared_ptr<cspot::Context> ctx, TrackReference& ref);

  enum class State { QUEUED, METADATA_LOADED, PLAYING };
};

class TaskQueue {
 public:
  TaskQueue(std::shared_ptr<cspot::Context> ctx);

  void updateTracks(TrackRef* tracks, size_t numberOfTracks,
                    size_t playingIndex);

 private:
  std::vector<QueuedTrack> tracks;
};
}  // namespace cspot