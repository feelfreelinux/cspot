#include "CliPlayer.h"
#include <functional>
#include <memory>
#include <mutex>

#include "BellUtils.h"
#include "CentralAudioBuffer.h"
#include "PortAudioSink.h"
#include "SpircHandler.h"

CliPlayer::CliPlayer(std::shared_ptr<cspot::SpircHandler> handler)
    : bell::Task("player", 1024, 0, 0) {
  this->handler = handler;
  this->audioSink = std::make_unique<PortAudioSink>();
  this->audioSink->setParams(44100, 2, 16);

  this->centralAudioBuffer = std::make_unique<bell::CentralAudioBuffer>(1024);

  auto hashFunc = std::hash<std::string_view>();

  this->handler->getTrackPlayer()->setDataCallback(
      [this, &hashFunc](uint8_t* data, size_t bytes, std::string_view trackId) {
        auto hash = hashFunc(trackId);

        return this->centralAudioBuffer->writePCM(data, bytes, hash);
      });
  this->isPaused = false;

  this->handler->setEventHandler(
      [this](std::unique_ptr<cspot::SpircHandler::Event> event) {
        switch (event->eventType) {
          case cspot::SpircHandler::EventType::PLAY_PAUSE:
            this->isPaused = std::get<bool>(event->data);
            break;
          case cspot::SpircHandler::EventType::FLUSH:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::SpircHandler::EventType::DISC:
            this->centralAudioBuffer->clearBuffer();
            // this->lastTrackHash = 0;
            break;
          case cspot::SpircHandler::EventType::SEEK:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::SpircHandler::EventType::PLAYBACK_START:
            this->centralAudioBuffer->clearBuffer();
          default:
            break;
        }
      });
  startTask();
}

void CliPlayer::feedData(uint8_t* data, size_t len) {}

void CliPlayer::runTask() {
  std::vector<uint8_t> outBuf = std::vector<uint8_t>(1024);

  std::scoped_lock lock(runningMutex);
  bell::CentralAudioBuffer::AudioChunk chunk;

  size_t lastHash = 0;

  while (isRunning) {
    if (!this->isPaused) {
      chunk = this->centralAudioBuffer->readChunk();

      if (chunk.pcmSize == 0) {
        BELL_SLEEP_MS(10);
        continue;
      } else {
        if (lastHash != chunk.trackHash) {
          std::cout << " Last hash " << lastHash << " new hash "
                    << chunk.trackHash << std::endl;
          lastHash = chunk.trackHash;
          this->handler->notifyAudioReachedPlayback();
        }
        this->audioSink->feedPCMFrames(chunk.pcmData, chunk.pcmSize);
      }
    } else {
      BELL_SLEEP_MS(10);
    }
  }
}

void CliPlayer::disconnect() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}
