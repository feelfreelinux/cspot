#pragma once

#include <memory>
#include <mutex>
#include "BellTask.h"
#include "CircularBuffer.h"
#include "PortAudioSink.h"
#include "SpircHandler.h"

class CliPlayer : public bell::Task {
 public:
  CliPlayer(std::shared_ptr<cspot::SpircHandler> spircHandler);
  void disconnect();
private:
  std::shared_ptr<cspot::SpircHandler> handler;
  std::unique_ptr<PortAudioSink> audioSink;
  std::unique_ptr<bell::CircularBuffer> circularBuffer;

  void feedData(uint8_t* data, size_t len);

  std::atomic<bool> isPaused = true;
  std::atomic<bool> isRunning = true;
  std::mutex runningMutex;

  void runTask() override;
};