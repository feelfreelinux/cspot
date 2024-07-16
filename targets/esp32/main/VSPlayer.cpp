#include "VSPlayer.h"

#include <cstdint>   // for uint8_t
#include <iostream>  // for operator<<, basic_ostream, endl, cout
#include <memory>    // for shared_ptr, make_shared, make_unique
#include <mutex>     // for scoped_lock
#include <variant>   // for get

#include "TrackPlayer.h"  // for TrackPlayer

VSPlayer::VSPlayer(std::shared_ptr<cspot::SpircHandler> handler,
                   std::shared_ptr<VS1053_SINK> vsSink) {
  this->handler = handler;
  this->vsSink = vsSink;
  this->vsSink->state_callback = [this](uint8_t state) {
    this->state_callback(state);
  };

  this->handler->getTrackPlayer()->setDataCallback(
      [this](uint8_t* data, size_t bytes, size_t trackId,
             bool STORAGE_VOLATILE) {
        if (!this->track) {
          this->track = std::make_shared<VS1053_TRACK>(this->vsSink.get(),
                                                       trackId, 4098 * 16);
          this->vsSink->new_track(this->track);
        }
        if (trackId != this->track->track_id) {
          this->vsSink->soft_stop_feed();
          this->track = std::make_shared<VS1053_TRACK>(this->vsSink.get(),
                                                       trackId, 4098 * 16);
          this->vsSink->new_track(this->track);
        }
        return this->track->feed_data(data, bytes, STORAGE_VOLATILE);
      },
      [this](size_t trackId) { return this->vsSink->track_seekable(trackId); },
      [this](size_t trackId) {
        return this->vsSink->spaces_available(trackId);
      });

  this->isPaused = false;

  this->handler->setEventHandler(
      [this](std::unique_ptr<cspot::SpircHandler::Event> event) {
        switch (event->eventType) {
          case cspot::SpircHandler::EventType::PLAY_PAUSE:
            if (std::get<bool>(event->data)) {
              if (this->track)
                this->vsSink->new_state(VS1053_TRACK::tsPlaybackPaused);
            } else {
              if (this->track)
                this->vsSink->new_state(VS1053_TRACK::tsPlaybackSeekable);
            }
            break;
          case cspot::SpircHandler::EventType::DISC:
            this->track = nullptr;
            this->vsSink->stop_feed();
            break;
          case cspot::SpircHandler::EventType::FLUSH:
            this->track->empty_feed();
            break;
          case cspot::SpircHandler::EventType::SEEK:
            break;
          case cspot::SpircHandler::EventType::PLAYBACK_START:
            this->isPaused = true;
            this->playlistEnd = false;
            break;
          case cspot::SpircHandler::EventType::DEPLETED:
            this->playlistEnd = true;
            this->track = nullptr;
            this->vsSink->stop_feed();
            break;
          case cspot::SpircHandler::EventType::VOLUME: {
            this->volume = std::get<int>(event->data);
            this->vsSink->feed_command([this](uint8_t) {
              this->vsSink->set_volume_logarithmic(this->volume);
            });
            break;
          }
          default:
            break;
        }
      });
}

void VSPlayer::state_callback(uint8_t state) {
  if (state == 1) {
    this->handler->notifyAudioReachedPlayback();
  }
  if (state == 7) {
    if (this->playlistEnd)
      this->handler->notifyAudioEnded();
    else
      this->handler->notifyAudioReachedPlaybackEnd();
  }
}

void VSPlayer::disconnect() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}
