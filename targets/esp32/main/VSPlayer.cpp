#include "VSPlayer.h"

#include <cstdint>   // for uint8_t
#include <iostream>  // for operator<<, basic_ostream, endl, cout
#include <memory>    // for shared_ptr, make_shared, make_unique
#include <mutex>     // for scoped_lock
#include <variant>   // for get

#include "TrackPlayer.h"  // for TrackPlayer

VSPlayer::VSPlayer(std::shared_ptr<cspot::DeviceStateHandler> handler,
                   std::shared_ptr<VS1053_SINK> vsSink) {
  this->handler = handler;
  this->vsSink = vsSink;
  this->vsSink->state_callback = [this](uint8_t state) {
    this->state_callback(state);
  };

  this->handler->trackPlayer->setDataCallback(
      [this](uint8_t* data, size_t bytes, size_t trackId,
             bool STORAGE_VOLATILE) {
        if (!this->track) {
          this->track = std::make_shared<VS1053_TRACK>(trackId, 4098 * 16);
          this->vsSink->new_track(this->track);
        }
        if (trackId != this->track->track_id) {
          this->vsSink->soft_stop_feed();
          this->track = std::make_shared<VS1053_TRACK>(trackId, 4098 * 16);
          this->vsSink->new_track(this->track);
        }
        return this->track->feed_data(data, bytes, STORAGE_VOLATILE);
      },
      [this](size_t trackId) { return this->vsSink->track_seekable(trackId); },
      [this](size_t trackId) {
        return this->vsSink->spaces_available(trackId);
      });

  this->isPaused = false;

  this->handler->stateCallback =
      [this](cspot::DeviceStateHandler::Command event) {
        switch (event.commandType) {
          case cspot::DeviceStateHandler::CommandType::PAUSE:
            if (this->track)
              this->vsSink->new_state(this->vsSink->tracks[0]->state,
                                      VS1053_TRACK::tsPlaybackPaused);
            break;
          case cspot::DeviceStateHandler::CommandType::PLAY:
            if (this->track)
              this->vsSink->new_state(this->vsSink->tracks[0]->state,
                                      VS1053_TRACK::tsPlaybackSeekable);
            break;
          case cspot::DeviceStateHandler::CommandType::DISC:
            this->track = nullptr;
            this->vsSink->delete_all_tracks();
            this->vsSink->stop_feed();
            //this->currentTrack = nullptr;
            this->futureTrack = nullptr;
            break;
          case cspot::DeviceStateHandler::CommandType::FLUSH:
            this->track->empty_feed();
            break;
          //case cspot::DeviceStateHandler::CommandType::SEEK:
          //break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK_START:
            break;
          case cspot::DeviceStateHandler::CommandType::PLAYBACK:
            this->isPaused = true;
            this->playlistEnd = false;
            if (this->currentTrack != nullptr)
              this->futureTrack =
                  std::get<std::shared_ptr<cspot::QueuedTrack>>(event.data);
            else
              this->currentTrack =
                  std::get<std::shared_ptr<cspot::QueuedTrack>>(event.data);
            break;
          case cspot::DeviceStateHandler::CommandType::DEPLETED:
            this->futureTrack = nullptr;
            this->vsSink->stop_feed();
            break;
          case cspot::DeviceStateHandler::CommandType::VOLUME: {
            this->volume = std::get<int32_t>(event.data);
            this->vsSink->feed_command([this](uint8_t) {
              this->vsSink->set_volume_logarithmic(this->volume);
            });
            break;
          }
          default:
            break;
        }
      };
}

void VSPlayer::state_callback(uint8_t state) {
  if (state == 1) {
    currentTrack->trackMetrics->startTrackPlaying(
        currentTrack->requestedPosition);
    this->handler->putPlayerState();
  }
  if (state == 7) {
    currentTrack->trackMetrics->endTrack();
    this->handler->ctx->playbackMetrics->sendEvent(currentTrack);
    if (futureTrack != nullptr) {
      currentTrack = futureTrack;
      futureTrack = nullptr;
    } else {
      currentTrack = nullptr;
      this->track = nullptr;
    }
  }
}

void VSPlayer::disconnect() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}
