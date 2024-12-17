#include "CliPlayer.h"

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

#include "BellDSP.h"             // for BellDSP, BellDSP::FadeEffect, BellDS...
#include "BellUtils.h"           // for BELL_SLEEP_MS
#include "CentralAudioBuffer.h"  // for CentralAudioBuffer::AudioChunk, Cent...
#include "DeviceStateHandler.h"  // for DeviceStateHandler, DeviceStateHandler::CommandType
#include "Logger.h"
#include "StreamInfo.h"   // for BitWidth, BitWidth::BW_16
#include "TrackPlayer.h"  // for TrackPlayer

CliPlayer::CliPlayer(std::unique_ptr<AudioSink> sink,
                     std::shared_ptr<cspot::DeviceStateHandler> handler)
    : bell::Task("player", 1024, 0, 0) {
  this->handler = handler;
  this->audioSink = std::move(sink);

  this->centralAudioBuffer =
      std::make_shared<bell::CentralAudioBuffer>(128 * 1024);

#ifndef BELL_DISABLE_CODECS
  this->dsp = std::make_shared<bell::BellDSP>(this->centralAudioBuffer);
#endif

  this->handler->trackPlayer->setDataCallback(
      [this](uint8_t* data, size_t bytes, size_t trackId,
             bool STORAGE_VOLATILE) {
        return this->centralAudioBuffer->writePCM(data, bytes, trackId);
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
          case cspot::DeviceStateHandler::CommandType::FLUSH: {
            this->centralAudioBuffer->clearBuffer();
            break;
          }
          case cspot::DeviceStateHandler::CommandType::DISC:
            this->centralAudioBuffer->clearBuffer();
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.clear();
            this->playlistEnd = true;
            break;
          case cspot::DeviceStateHandler::CommandType::SEEK:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::DeviceStateHandler::CommandType::SKIP_NEXT:
          case cspot::DeviceStateHandler::CommandType::SKIP_PREV:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK_START:
            this->centralAudioBuffer->clearBuffer();
            if (tracks.size())
              tracks.clear();
            this->isPaused = false;
            this->playlistEnd = false;
            break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK:
            tracks.push_back(
                std::get<std::shared_ptr<cspot::QueuedTrack>>(event.data));
            this->isPaused = false;
            this->playlistEnd = false;
            break;
          case cspot::DeviceStateHandler::CommandType::DEPLETED:
            this->centralAudioBuffer->clearBuffer();
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

void CliPlayer::feedData(uint8_t* data, size_t len) {}

void CliPlayer::runTask() {
  std::vector<uint8_t> outBuf = std::vector<uint8_t>(1024);

  std::scoped_lock lock(runningMutex);
  bell::CentralAudioBuffer::AudioChunk* chunk;

  size_t lastHash = 0;

  while (isRunning) {
    if (!this->isPaused) {
      chunk = this->centralAudioBuffer->readChunk();

      if (this->pauseRequested) {
        this->pauseRequested = false;
        std::cout << "Pause requested!" << std::endl;
#ifndef BELL_DISABLE_CODECS
        auto effect = std::make_unique<bell::BellDSP::FadeEffect>(
            44100 / 2, false, [this]() { this->isPaused = true; });
        this->dsp->queryInstantEffect(std::move(effect));
#else
        this->isPaused = true;
#endif
      }

      if (!chunk || chunk->pcmSize == 0) {
        if (this->playlistEnd) {
          this->playlistEnd = false;
          if (tracks.size()) {
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.clear();
            this->handler->putPlayerState();
          }
          lastHash = 0;
        }
        BELL_SLEEP_MS(10);
        continue;
      } else {
        if (lastHash != chunk->trackHash) {
          if (lastHash) {
            tracks.at(0)->trackMetrics->endTrack();
            this->handler->ctx->playbackMetrics->sendEvent(tracks[0]);
            tracks.pop_front();
            this->handler->trackPlayer->onTrackEnd(true);
          }
          lastHash = chunk->trackHash;
          tracks.at(0)->trackMetrics->startTrackPlaying(
              tracks.at(0)->requestedPosition);
          this->handler->putPlayerState();
        }

#ifndef BELL_DISABLE_CODECS
        this->dsp->process(chunk->pcmData, chunk->pcmSize, 2, 44100,
                           bell::BitWidth::BW_16);
#endif
        this->audioSink->feedPCMFrames(chunk->pcmData, chunk->pcmSize);
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
