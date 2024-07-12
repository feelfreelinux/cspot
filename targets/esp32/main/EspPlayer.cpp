#include "EspPlayer.h"

#include <cstdint>      // for uint8_t
#include <functional>   // for __base
#include <iostream>     // for operator<<, basic_ostream, endl, cout
#include <memory>       // for shared_ptr, make_shared, make_unique
#include <mutex>        // for scoped_lock
#include <string_view>  // for hash, string_view
#include <type_traits>  // for remove_extent_t
#include <utility>      // for move
#include <variant>      // for get
#include <vector>       // for vector

#include "BellUtils.h"  // for BELL_SLEEP_MS
#include "CircularBuffer.h"
#include "Logger.h"
#include "SpircHandler.h"  // for SpircHandler, SpircHandler::EventType
#include "StreamInfo.h"    // for BitWidth, BitWidth::BW_16
#include "TrackPlayer.h"   // for TrackPlayer

EspPlayer::EspPlayer(std::unique_ptr<AudioSink> sink,
                     std::shared_ptr<cspot::SpircHandler> handler)
    : bell::Task("player", 32 * 1024, 0, 1) {
  this->handler = handler;
  this->audioSink = std::move(sink);

  this->circularBuffer = std::make_shared<bell::CircularBuffer>(1024 * 128);

  auto hashFunc = std::hash<std::string_view>();

  this->handler->getTrackPlayer()->setDataCallback(
      [this, &hashFunc](uint8_t* data, size_t bytes, std::string_view trackId) {
        auto hash = hashFunc(trackId);
        this->feedData(data, bytes, hash);
        return bytes;
      });

  this->isPaused = false;

  this->handler->setEventHandler(
      [this, &hashFunc](std::unique_ptr<cspot::SpircHandler::Event> event) {
        switch (event->eventType) {
          case cspot::SpircHandler::EventType::PLAY_PAUSE:
            if (std::get<bool>(event->data)) {
              this->pauseRequested = true;
            } else {
              this->isPaused = false;
              this->pauseRequested = false;
            }
            break;
          case cspot::SpircHandler::EventType::DISC:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::SpircHandler::EventType::FLUSH:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::SpircHandler::EventType::SEEK:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::SpircHandler::EventType::PLAYBACK_START:
            this->isPaused = true;
            this->playlistEnd = false;
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::SpircHandler::EventType::DEPLETED:
            this->playlistEnd = true;
            break;
          case cspot::SpircHandler::EventType::VOLUME: {
            int volume = std::get<int>(event->data);
            break;
          }
          default:
            break;
        }
      });
  startTask();
}

void EspPlayer::feedData(uint8_t* data, size_t len, size_t trackId) {
  size_t toWrite = len;

  while (toWrite > 0) {
    this->current_hash = trackId;
    size_t written =
        this->circularBuffer->write(data + (len - toWrite), toWrite);
    if (written == 0) {
      BELL_SLEEP_MS(10);
    }

    toWrite -= written;
  }
}

void EspPlayer::runTask() {
  std::vector<uint8_t> outBuf = std::vector<uint8_t>(1024);

  std::scoped_lock lock(runningMutex);

  size_t lastHash = 0;

  while (isRunning) {
    if (!this->isPaused) {
      size_t read = this->circularBuffer->read(outBuf.data(), outBuf.size());
      if (this->pauseRequested) {
        this->pauseRequested = false;
        std::cout << "Pause requested!" << std::endl;
        this->isPaused = true;
      }

      this->audioSink->feedPCMFrames(outBuf.data(), read);

      if (read == 0) {
        if (this->playlistEnd) {
          this->handler->notifyAudioEnded();
          this->playlistEnd = false;
        }
        BELL_SLEEP_MS(10);
        continue;
      } else {
        if (lastHash != current_hash) {
          lastHash = current_hash;
          this->handler->notifyAudioReachedPlayback();
        }
      }
    } else {
      BELL_SLEEP_MS(100);
    }
  }
}

void EspPlayer::disconnect() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}
