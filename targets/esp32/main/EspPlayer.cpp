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
#include "DeviceStateHandler.h"  // for SpircHandler, DeviceStateHandler::CommandType
#include "Logger.h"
#include "StreamInfo.h"   // for BitWidth, BitWidth::BW_16
#include "TrackPlayer.h"  // for TrackPlayer

EspPlayer::EspPlayer(std::unique_ptr<AudioSink> sink,
                     std::shared_ptr<cspot::DeviceStateHandler> handler)
    : bell::Task("player", 32 * 1024, 0, 1) {
  this->handler = handler;
  this->audioSink = std::move(sink);

  this->circularBuffer = std::make_shared<bell::CircularBuffer>(1024 * 128);

  this->handler->trackPlayer->setDataCallback([this](uint8_t* data,
                                                     size_t bytes,
#ifdef CONFIG_BELL_NOCODEC
                                                     bool STORAGE_VOLATILE,
#endif
                                                     size_t trackId) {
    this->feedData(data, bytes, trackId);
    return bytes;
  });

  this->isPaused = false;

  this->handler->stateCallback =
      [this](cspot::DeviceStateHandler::Command event) {
        switch (event.commandType) {
          case cspot::DeviceStateHandler::CommandType::PAUSE:
            this->pauseRequested = true;
            break;
          case cspot::DeviceStateHandler::CommandType::PLAY:
            this->isPaused = false;
            this->pauseRequested = false;
            break;
          case cspot::DeviceStateHandler::CommandType::DISC:
            this->circularBuffer->emptyBuffer();
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.clear();
            this->playlistEnd = true;
            break;
          case cspot::DeviceStateHandler::CommandType::FLUSH:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::DeviceStateHandler::CommandType::SEEK:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::DeviceStateHandler::CommandType::SKIP_NEXT:
          case cspot::DeviceStateHandler::CommandType::SKIP_PREV:
            this->circularBuffer->emptyBuffer();
            break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK_START:
            this->circularBuffer->emptyBuffer();
            this->isPaused = false;
            this->playlistEnd = false;
            if (tracks.size())
              tracks.clear();
            break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK:
            tracks.push_back(
                std::get<std::shared_ptr<cspot::QueuedTrack>>(event.data));
            this->isPaused = false;
            this->playlistEnd = false;
            break;
          case cspot::DeviceStateHandler::CommandType::DEPLETED:
            this->circularBuffer->emptyBuffer();
            this->playlistEnd = true;
            break;
          case cspot::DeviceStateHandler::CommandType::VOLUME: {
            int volume = std::get<int32_t>(event.data);
            break;
          }
          default:
            break;
        }
      };
  startTask();
}

void EspPlayer::feedData(uint8_t* data, size_t len, size_t trackId) {
  size_t toWrite = len;

  if (!len) {
    tracks.at(0)->trackMetrics->endTrack();
    this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
    if (this->playlistEnd) {
      tracks.clear();
    }
  } else
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
        this->isPaused = true;
      }

      this->audioSink->feedPCMFrames(outBuf.data(), read);

      if (read == 0) {
        if (this->playlistEnd) {
          this->playlistEnd = false;
          if (tracks.size()) {
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.clear();
          }
          lastHash = 0;
        }
        BELL_SLEEP_MS(10);
        continue;
      } else {
        if (lastHash != current_hash) {
          if (lastHash) {
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.pop_front();
            this->handler->trackPlayer->eofCallback(true);
          }
          lastHash = current_hash;
          tracks.at(0)->trackMetrics->startTrackPlaying(
              tracks.at(0)->requestedPosition);
          this->handler->putPlayerState();
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
