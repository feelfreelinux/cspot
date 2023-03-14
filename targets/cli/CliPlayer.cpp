#include "CliPlayer.h"
#include <functional>
#include <memory>
#include <mutex>

#include "BellUtils.h"
#include "CentralAudioBuffer.h"
#include "SpircHandler.h"

#if defined(CSPOT_ENABLE_ALSA_SINK)
#include "ALSAAudioSink.h"
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
#include "PortAudioSink.h"
#else
#include "NamedPipeAudioSink.h"
#endif

CliPlayer::CliPlayer(std::shared_ptr<cspot::SpircHandler> handler)
    : bell::Task("player", 1024, 0, 0) {
  this->handler = handler;
#ifdef CSPOT_ENABLE_ALSA_SINK
  this->audioSink = std::make_unique<ALSAAudioSink>();
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
  this->audioSink = std::make_unique<PortAudioSink>();
#else
  this->audioSink = std::make_unique<NamedPipeAudioSink>();
#endif

#if defined(BELL_SINK_PORTAUDIO)
  this->audioSink = std::make_unique<PortAudioSink>();
#elif defined(BELL_SINK_ALSA)
  this->audioSink = std::make_unique<AlsaSink>();
#endif
  this->audioSink->setParams(44100, 2, 16);

  this->centralAudioBuffer = std::make_shared<bell::CentralAudioBuffer>(1024);

#ifndef BELL_DISABLE_CODECS
  this->dsp = std::make_shared<bell::BellDSP>(this->centralAudioBuffer);
#endif

  auto hashFunc = std::hash<std::string_view>();

  this->handler->getTrackPlayer()->setDataCallback(
      [this, &hashFunc](uint8_t* data, size_t bytes, std::string_view trackId, size_t sequence) {
        auto hash = hashFunc(trackId);

        return this->centralAudioBuffer->writePCM(data, bytes, hash);
      });

  this->isPaused = false;

  this->handler->setEventHandler(
      [this](std::unique_ptr<cspot::SpircHandler::Event> event) {
        switch (event->eventType) {
          case cspot::SpircHandler::EventType::PLAY_PAUSE:
            if (std::get<bool>(event->data)) {
              this->pauseRequested = true;
            } else {
              this->isPaused = false;
            }
            break;
          case cspot::SpircHandler::EventType::FLUSH:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::SpircHandler::EventType::DISC:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::SpircHandler::EventType::SEEK:
            this->centralAudioBuffer->clearBuffer();
            break;
          case cspot::SpircHandler::EventType::PLAYBACK_START:
            this->centralAudioBuffer->clearBuffer();
            break;
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
  bell::CentralAudioBuffer::AudioChunk* chunk;

  size_t lastHash = 0;

  while (isRunning) {
    if (!this->isPaused) {
      chunk = this->centralAudioBuffer->readChunk();

      if (this->pauseRequested) {
        this->pauseRequested = false;
        std::cout << "Pause requsted!" << std::endl;
#ifndef BELL_DISABLE_CODECS
        auto effect = std::make_unique<bell::BellDSP::FadeEffect>(
            44100 / 2, false, [this]() { this->isPaused = true; });
        this->dsp->queryInstantEffect(std::move(effect));
#endif
      }

      if (!chunk || chunk->pcmSize == 0) {
        BELL_SLEEP_MS(10);
        continue;
      } else {
        if (lastHash != chunk->trackHash) {
          std::cout << " Last hash " << lastHash << " new hash "
                    << chunk->trackHash << std::endl;
          lastHash = chunk->trackHash;
          this->handler->notifyAudioReachedPlayback();
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
