#pragma once

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t
#include <atomic>    // for atomic
#include <memory>    // for shared_ptr, unique_ptr
#include <mutex>     // for mutex
#include <string>    // for string

#include "AudioSink.h"           // for AudioSink
#include "BellTask.h"            // for Task
#include "DeviceStateHandler.h"  // for DeviceStateHandler

namespace bell {
class CircularBuffer;
}  // namespace bell
namespace cspot {
class DeviceStateHandler;
}  // namespace cspot

class EspPlayer : public bell::Task {
 public:
  EspPlayer(std::shared_ptr<AudioSink> sink,
            std::shared_ptr<cspot::DeviceStateHandler> handler);
  void disconnect();

 private:
  std::string currentTrackId;
  std::shared_ptr<cspot::DeviceStateHandler> handler;
  std::shared_ptr<AudioSink> audioSink;
  std::shared_ptr<bell::CircularBuffer> circularBuffer;
  std::deque<std::shared_ptr<cspot::QueuedTrack>> tracks = {};
  void feedData(uint8_t* data, size_t len, size_t);

  std::atomic<bool> pauseRequested = false;
  std::atomic<bool> isPaused = true;
  std::atomic<bool> isRunning = true;
  std::mutex runningMutex;
  std::atomic<bool> playlistEnd = false;
  size_t current_hash;

  void runTask() override;
};
