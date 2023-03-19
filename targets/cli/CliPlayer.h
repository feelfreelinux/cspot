#pragma once

#include <memory>
#include <mutex>
#include "BellTask.h"
#include "CentralAudioBuffer.h"
#include "SpircHandler.h"
#include "BellDSP.h"
#include "AudioSink.h"

class CliPlayer : public bell::Task {
 public:
  CliPlayer(std::shared_ptr<cspot::SpircHandler> spircHandler);
  void disconnect();
private:
  std::string currentTrackId;
  std::shared_ptr<cspot::SpircHandler> handler;
  std::shared_ptr<bell::BellDSP> dsp;
  std::unique_ptr<AudioSink> audioSink;
  std::shared_ptr<bell::CentralAudioBuffer> centralAudioBuffer;

  void feedData(uint8_t* data, size_t len);

  std::atomic<bool> pauseRequested = false;
  std::atomic<bool> isPaused = true;
  std::atomic<bool> isRunning = true;
  std::mutex runningMutex;

  void runTask() override;
};
