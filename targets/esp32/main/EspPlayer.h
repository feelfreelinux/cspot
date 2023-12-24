#pragma once

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t
#include <atomic>    // for atomic
#include <memory>    // for shared_ptr, unique_ptr
#include <mutex>     // for mutex
#include <string>    // for string

#include "AudioSink.h"  // for AudioSink
#include "BellTask.h"   // for Task

namespace bell {
class CircularBuffer;
}  // namespace bell
namespace cspot {
class SpircHandler;
}  // namespace cspot

class EspPlayer : public bell::Task {
 public:
  EspPlayer(std::unique_ptr<AudioSink> sink,
            std::shared_ptr<cspot::SpircHandler> spircHandler);
  void disconnect();

 private:
  std::string currentTrackId;
  std::shared_ptr<cspot::SpircHandler> handler;
  std::unique_ptr<AudioSink> audioSink;
  std::shared_ptr<bell::CircularBuffer> circularBuffer;
  void feedData(uint8_t* data, size_t len, size_t);


  std::atomic<bool> pauseRequested = false;
  std::atomic<bool> isPaused = true;
  std::atomic<bool> isRunning = true;
  std::mutex runningMutex;
  std::atomic<bool> playlistEnd = false;
  size_t current_hash;

  void runTask() override;
};
