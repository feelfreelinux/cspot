#pragma once

#include <memory>
#include "BellTask.h"
#include "CircularBuffer.h"
#include "PortAudioSink.h"
#include "SpircHandler.h"

class CliPlayer : public bell::Task {
 public:
  CliPlayer(std::shared_ptr<cspot::SpircHandler> spircHandler);
private:
  std::shared_ptr<cspot::SpircHandler> handler;
  std::unique_ptr<PortAudioSink> audioSink;
  std::unique_ptr<bell::CircularBuffer> circularBuffer;

  void feedData(uint8_t* data, size_t len);

  std::atomic<bool> isPaused = true;

  void runTask() override;
};