#include "TrackPlayer.h"

#include <mutex>        // for mutex, scoped_lock
#include <string>       // for string
#include <type_traits>  // for remove_extent_t
#include <vector>       // for vector, vector<>::value_type

#include "BellLogger.h"  // for AbstractLogger
#include "BellUtils.h"   // for BELL_SLEEP_MS
#include "CSpotContext.h"
#include "Logger.h"            // for CSPOT_LOG
#include "Packet.h"            // for cspot
#include "TrackQueue.h"        // for CDNTrackStream, CDNTrackStream::TrackInfo
#include "WrappedSemaphore.h"  // for WrappedSemaphore

#ifndef CONFIG_BELL_NOCODEC
#ifdef BELL_VORBIS_FLOAT
#define VORBIS_SEEK(file, position) \
  (ov_time_seek(file, (double)position / 1000))
#define VORBIS_READ(file, buffer, bufferSize, section) \
  (ov_read(file, buffer, bufferSize, 0, 2, 1, section))
#else
#define VORBIS_SEEK(file, position) (ov_time_seek(file, position))
#define VORBIS_READ(file, buffer, bufferSize, section) \
  (ov_read(file, buffer, bufferSize, section))
#endif
#endif

namespace cspot {
struct Context;
struct TrackReference;
class PlaybackMetrics;
}  // namespace cspot

using namespace cspot;

#ifndef CONFIG_BELL_NOCODEC
static size_t vorbisReadCb(void* ptr, size_t size, size_t nmemb,
                           TrackPlayer* self) {
  return self->_vorbisRead(ptr, size, nmemb);
}

static int vorbisCloseCb(TrackPlayer* self) {
  return self->_vorbisClose();
}

static int vorbisSeekCb(TrackPlayer* self, int64_t offset, int whence) {

  return self->_vorbisSeek(offset, whence);
}

static long vorbisTellCb(TrackPlayer* self) {
  return self->_vorbisTell();
}
#endif

TrackPlayer::TrackPlayer(std::shared_ptr<cspot::Context> ctx,
                         std::shared_ptr<cspot::TrackQueue> trackQueue,
                         EOFCallback eof, TrackLoadedCallback trackLoaded)
    : bell::Task("cspot_player", 48 * 1024, 5, 1) {
  this->ctx = ctx;
  this->eofCallback = eof;
  this->trackLoaded = trackLoaded;
  this->trackQueue = trackQueue;
  this->playbackSemaphore = std::make_unique<bell::WrappedSemaphore>(5);

#ifndef CONFIG_BELL_NOCODEC
  // Initialize vorbis callbacks
  vorbisFile = {};
  vorbisCallbacks = {
      (decltype(ov_callbacks::read_func))&vorbisReadCb,
      (decltype(ov_callbacks::seek_func))&vorbisSeekCb,
      (decltype(ov_callbacks::close_func))&vorbisCloseCb,
      (decltype(ov_callbacks::tell_func))&vorbisTellCb,
  };
#endif
}

TrackPlayer::~TrackPlayer() {
  isRunning = false;
  resetState();
  std::scoped_lock lock(runningMutex);
}

void TrackPlayer::start() {
  if (!isRunning) {
    isRunning = true;
    startTask();
    this->ctx->playbackMetrics->start_reason = PlaybackMetrics::REMOTE;
    this->ctx->playbackMetrics->start_source = "unknown";
  } else
    this->ctx->playbackMetrics->end_reason = PlaybackMetrics::END_PLAY;
}

void TrackPlayer::stop() {
  isRunning = false;
  resetState();
  std::scoped_lock lock(runningMutex);
}

void TrackPlayer::resetState(bool paused) {
  // Mark for reset
  this->pendingReset = true;
  this->currentSongPlaying = false;
  this->startPaused = paused;

  std::scoped_lock lock(dataOutMutex);

  CSPOT_LOG(info, "Resetting state");
}

void TrackPlayer::seekMs(size_t ms) {
#ifndef CONFIG_BELL_NOCODEC
  if (inFuture) {
    // We're in the middle of the next track, so we need to reset the player in order to seek
    resetState();
  }
#endif

  CSPOT_LOG(info, "Seeking...");
  this->pendingSeekPositionMs = ms;
}

void TrackPlayer::runTask() {
  std::scoped_lock lock(runningMutex);

  std::shared_ptr<QueuedTrack> track = nullptr, newTrack = nullptr;

  int trackOffset = 0;
  size_t tracksPlayed = 1;
  bool eof = false;
  bool endOfQueueReached = false;

  while (isRunning) {
    bool properStream = true;
    // Ensure we even have any tracks to play
    if (!this->trackQueue->hasTracks() ||
        (!pendingReset && endOfQueueReached && trackQueue->isFinished())) {
      this->trackQueue->playableSemaphore->twait(300);
      continue;
    }

    // Last track was interrupted, reset to default
    if (pendingReset) {
      track = nullptr;
      pendingReset = false;
      inFuture = false;
    }

    endOfQueueReached = false;

    // Wait 800ms. If next reset is requested in meantime, restart the queue.
    // Gets rid of excess actions during rapid queueing
    BELL_SLEEP_MS(50);

    if (pendingReset) {
      continue;
    }

    newTrack = trackQueue->consumeTrack(track, trackOffset);
    this->trackQueue->update_ghost_tracks(trackOffset);

    if (newTrack == nullptr) {
      if (trackOffset == -1) {
        // Reset required
        track = nullptr;
      }

      BELL_SLEEP_MS(100);
      continue;
    }

    track = newTrack;
    track->trackMetrics = std::make_shared<TrackMetrics>(this->ctx);
    this->ctx->playbackMetrics->trackMetrics = track->trackMetrics;

    inFuture = trackOffset > 0;

    if (track->state != QueuedTrack::State::READY) {
      track->loadedSemaphore->twait(5000);

      if (track->state != QueuedTrack::State::READY) {
        CSPOT_LOG(error, "Track failed to load, skipping it");
        this->eofCallback(false);
        continue;
      }
    }

    CSPOT_LOG(info, "Got track ID=%s", track->identifier.c_str());

    currentSongPlaying = true;

    {
      std::scoped_lock lock(playbackMutex);
      bool skipped = 0;

      track->trackMetrics->startTrack();
      currentTrackStream = track->getAudioFile();

      // Open the stream
      currentTrackStream->openStream();

      if (pendingReset || !currentSongPlaying) {
        continue;
      }
      track->trackMetrics->startTrackDecoding();
      track->trackMetrics->track_size = currentTrackStream->getSize();

#ifndef CONFIG_BELL_NOCODEC
      if (trackOffset == 0 && pendingSeekPositionMs == 0) {
        this->trackLoaded(track, startPaused);
        startPaused = false;
      }

      int32_t r =
          ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
#else
      size_t start_offset = 0;
      size_t write_offset = 0;
      while (!start_offset) {
        size_t ret = this->currentTrackStream->readBytes(&pcmBuffer[0],
                                                         pcmBuffer.size());
        size_t written = 0;
        size_t toWrite = ret;
        while (toWrite) {
          written = dataCallback(pcmBuffer.data() + (ret - toWrite), toWrite,
                                 tracksPlayed, 0);
          if (written == 0) {
            BELL_SLEEP_MS(1000);
          }
          toWrite -= written;
        }
        track->written_bytes += ret;
        start_offset = seekable_callback(tracksPlayed);
        if (this->spaces_available(tracksPlayed) < pcmBuffer.size()) {
          BELL_SLEEP_MS(50);
          continue;
        }
      }
      float duration_lambda = 1.0 *
                              (currentTrackStream->getSize() - start_offset) /
                              track->trackInfo.duration;
#endif
      if (pendingSeekPositionMs > 0) {
        track->requestedPosition = pendingSeekPositionMs;
#ifdef CONFIG_BELL_NOCODEC
        pendingSeekPositionMs = 0;
#endif
      }
      ctx->playbackMetrics->end_reason = PlaybackMetrics::REMOTE;

      if (track->requestedPosition > 0) {
#ifndef CONFIG_BELL_NOCODEC
        VORBIS_SEEK(&vorbisFile, track->requestedPosition);
#else
        size_t seekPosition =
            track->requestedPosition * duration_lambda + start_offset;
        currentTrackStream->seek(seekPosition);
        skipped = true;
#endif
      }

      eof = false;
      track->loading = true;
      //in case of a repeatedtrack, set requested position to 0
      track->requestedPosition = 0;

      CSPOT_LOG(info, "Playing");

      while (!eof && currentSongPlaying) {
        // Execute seek if needed
        if (pendingSeekPositionMs > 0) {
          uint32_t seekPosition = pendingSeekPositionMs;

          // Seek to the new position
#ifndef CONFIG_BELL_NOCODEC
          VORBIS_SEEK(&vorbisFile, seekPosition);
#else
          seekPosition = seekPosition * duration_lambda + start_offset;
          currentTrackStream->seek(seekPosition);
          track->trackMetrics->newPosition(pendingSeekPositionMs);
          skipped = true;
#endif

          // Reset the pending seek position
          pendingSeekPositionMs = 0;
        }

        long ret =
#ifdef CONFIG_BELL_NOCODEC
            this->currentTrackStream->readBytes(&pcmBuffer[0],
                                                pcmBuffer.size());
#else
            VORBIS_READ(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(),
                        &currentSection);
#endif

        if (ret < 0) {
          CSPOT_LOG(error, "An error has occured in the stream %d", ret);
          currentSongPlaying = false;
          properStream = false;
        } else {
          if (ret == 0) {
            CSPOT_LOG(info, "EOF");
            // and done :)
            eof = true;
          }
          if (this->dataCallback != nullptr) {
            auto toWrite = ret;

            while (!eof && currentSongPlaying && !pendingReset && toWrite > 0) {
              int written = 0;
              {
                std::scoped_lock dataOutLock(dataOutMutex);
                // If reset happened during playback, return
                if (!currentSongPlaying || pendingReset)
                  break;
#ifdef CONFIG_BELL_NOCODEC
                if (skipped) {
                  // Reset the pending seek position
                  skipped = 0;
                }
#endif
                written = dataCallback(pcmBuffer.data() + (ret - toWrite),
                                       toWrite, tracksPlayed
#ifdef CONFIG_BELL_NOCODEC
                                       ,
                                       skipped
#endif
                );
              }
              if (written == 0) {
                BELL_SLEEP_MS(50);
              }
              toWrite -= written;
            }
            track->written_bytes += ret;
          }
        }
      }
      tracksPlayed++;
#ifndef CONFIG_BELL_NOCODEC
      ov_clear(&vorbisFile);
#endif

      CSPOT_LOG(info, "Playing done");

      // always move back to LOADING (ensure proper seeking after last track has been loaded)
      currentTrackStream = nullptr;
      track->loading = false;
    }

    if (eof) {
      if (trackQueue->isFinished()) {
        endOfQueueReached = true;
      }
    }
    this->eofCallback(properStream);
  }
}

#ifndef CONFIG_BELL_NOCODEC
size_t TrackPlayer::_vorbisRead(void* ptr, size_t size, size_t nmemb) {
  if (this->currentTrackStream == nullptr) {
    return 0;
  }
  return this->currentTrackStream->readBytes((uint8_t*)ptr, nmemb * size);
}

size_t TrackPlayer::_vorbisClose() {
  return 0;
}

int TrackPlayer::_vorbisSeek(int64_t offset, int whence) {
  if (this->currentTrackStream == nullptr) {
    return 0;
  }
  switch (whence) {
    case 0:
      this->currentTrackStream->seek(offset);  // Spotify header offset
      break;
    case 1:
      this->currentTrackStream->seek(this->currentTrackStream->getPosition() +
                                     offset);
      break;
    case 2:
      this->currentTrackStream->seek(this->currentTrackStream->getSize() +
                                     offset);
      break;
  }

  return 0;
}

long TrackPlayer::_vorbisTell() {
  if (this->currentTrackStream == nullptr) {
    return 0;
  }
  return this->currentTrackStream->getPosition();
}
#endif

void TrackPlayer::setDataCallback(DataCallback callback,
                                  SeekableCallback seekable_callback,
                                  SeekableCallback spaces_available) {
  this->dataCallback = callback;
#ifdef CONFIG_BELL_NOCODEC
  this->seekable_callback = seekable_callback;
  this->spaces_available = spaces_available;
#endif
}
