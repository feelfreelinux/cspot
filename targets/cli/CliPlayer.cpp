#include "CliPlayer.h"
#include <memory>
#include <mutex>
#include "BellUtils.h"
#include "CircularBuffer.h"
#include "PortAudioSink.h"
#include "SpircHandler.h"

CliPlayer::CliPlayer(std::shared_ptr<cspot::SpircHandler> handler)
    : bell::Task("player", 1024, 0, 0) {
  this->handler = handler;
  this->audioSink = std::make_unique<PortAudioSink>();
  this->audioSink->setParams(44100, 2, 16);

  this->circularBuffer = std::make_unique<bell::CircularBuffer>(1024 * 128 * 8);

  this->handler->getTrackPlayer()->setDataCallback(
      [this](uint8_t* data, size_t bytes) { this->feedData(data, bytes); });
  this->isPaused = false;

  this->handler->setEventHandler([this] (std::unique_ptr<cspot::SpircHandler::Event> event) {
    switch (event->eventType) {
      case cspot::SpircHandler::EventType::PLAY_PAUSE:
        this->isPaused = std::get<bool>(event->data);
        break;
      case cspot::SpircHandler::EventType::FLUSH:
        this->circularBuffer->emptyBuffer();
        break;
      case cspot::SpircHandler::EventType::SEEK:
        this->circularBuffer->emptyBuffer();
        break;
      case cspot::SpircHandler::EventType::PLAYBACK_START:
        this->circularBuffer->emptyBuffer();
      default:
        break;
    }
  });
  startTask();
}

void CliPlayer::feedData(uint8_t* data, size_t len) {
  size_t toWrite = len;

  while (toWrite > 0) {
    size_t written =
        this->circularBuffer->write(data + (len - toWrite), toWrite);
    if (written == 0) {
      BELL_SLEEP_MS(10);
    }

    toWrite -= written;
  }
}

void CliPlayer::runTask() {
  std::vector<uint8_t> outBuf = std::vector<uint8_t>(1024);

  std::scoped_lock lock(runningMutex);
  while (isRunning) {
    if (!this->isPaused) {
      size_t read = this->circularBuffer->read(outBuf.data(), outBuf.size());
      this->audioSink->feedPCMFrames(outBuf.data(), read);
    } else {
      BELL_SLEEP_MS(10);
    }
  }
}

void CliPlayer::disconnect() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}