#include "TrackPlayer.h"

#include <mutex>        // for mutex, scoped_lock
#include <string>       // for string
#include <type_traits>  // for remove_extent_t
#include <vector>       // for vector, vector<>::value_type

#include "BellLogger.h"  // for AbstractLogger
#include "BellUtils.h"   // for BELL_SLEEP_MS
#include "CSpotContext.h"
#include "EventManager.h"
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
                         TrackEndedCallback onTrackEnd,
                         TrackChangedCallback onTrackChanged,
                         bool* repeating_track)
    : bell::Task("cspot_player", 48 * 1024, 5, 1) {
  this->ctx = ctx;
  this->onTrackEnd = onTrackEnd;
  this->onTrackChanged = onTrackChanged;
  this->trackQueue = trackQueue;
  this->playbackSemaphore = std::make_unique<bell::WrappedSemaphore>(5);
  repeating_track_ = repeating_track;

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

void TrackPlayer::seekMs(size_t ms, bool loading) {
#ifndef CONFIG_BELL_NOCODEC
  if (!loading) {
    // We're in the middle of the next track, so we need to reset the player in order to seek
    CSPOT_LOG(info, "Resetting state");
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
    this->trackQueue->playableSemaphore->wait();

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
    if (!*repeating_track_ || newTrack == nullptr)
      newTrack = trackQueue->consumeTrack(track, trackOffset);

    if (newTrack == nullptr) {
      if (trackOffset == -1) {
        // Reset required
        track = nullptr;
      }
      CSPOT_LOG(error, "NULLPTR");

      BELL_SLEEP_MS(100);
      continue;
    }

    track = newTrack;
    this->ctx->playbackMetrics->trackMetrics = track->trackMetrics;

    inFuture = trackOffset > 0;
    uint8_t retries = 10;
    while (track->state != QueuedTrack::State::READY &&
           track->state != QueuedTrack::State::FAILED && retries-- > 0) {
      BELL_SLEEP_MS(100);
      CSPOT_LOG(error, "Track in state %i", (int)track->state);
    }
    if (track->state != QueuedTrack::State::READY) {
      CSPOT_LOG(error, "Track failed to load, skipping it");
      if (track->ref.removed != NULL)
        this->onTrackChanged(track, false);
      this->onTrackEnd(true);
      continue;
    }
    track->playingTrackIndex = tracksPlayed;
    currentSongPlaying = true;
    track->trackMetrics->startTrack();
    retries = 3;
    {
      std::scoped_lock lock(playbackMutex);
      bool skipped = 0;

      currentTrackStream = track->getAudioFile();
      // Open the stream
#ifndef CONFIG_BELL_NOCODEC
      currentTrackStream->openStream();
#else
      ssize_t start_offset = 0;
      uint8_t* headerBuf = currentTrackStream->openStream(start_offset);
      if (start_offset < 0) {
        CSPOT_LOG(error, "Track failed to load, skipping it");
        this->onTrackEnd(true);
        continue;
      }
#endif
      if (pendingReset || !currentSongPlaying) {
        continue;
      }
      track->trackMetrics->startTrackDecoding();
      track->trackMetrics->track_size = currentTrackStream->getSize();

      this->onTrackChanged(track, true);
      startPaused = false;

#ifndef CONFIG_BELL_NOCODEC
      int32_t r =
          ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
#else
      size_t toWrite = start_offset;
      while (toWrite) {
        size_t written = dataCallback(headerBuf + (start_offset - toWrite),
                                      toWrite, tracksPlayed, 0);
        if (written == 0) {
          BELL_SLEEP_MS(10);
        }
        toWrite -= written;
      }

      track->written_bytes += start_offset;
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

      CSPOT_LOG(info, "Playing");

      while (!eof && currentSongPlaying) {
        // Execute seek if needed
        if (pendingSeekPositionMs > 0) {
          track->requestedPosition = pendingSeekPositionMs;

          // Seek to the new position
#ifndef CONFIG_BELL_NOCODEC
          VORBIS_SEEK(&vorbisFile, track->requestedPosition);
#else
          uint32_t seekPosition = track->requestedPosition * duration_lambda +
                                  seekable_callback(tracksPlayed);
          currentTrackStream->seek(seekPosition);
          skipped = true;
#endif
          track->trackMetrics->newPosition(pendingSeekPositionMs);
          // Reset the pending seek position
          pendingSeekPositionMs = 0;
          this->onTrackChanged(track, false);
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
          if (retries == 0) {
            CSPOT_LOG(error, "Track failed to reload, skipping it");
            currentSongPlaying = false;
            properStream = false;
            eof = true;
          } else {
            CSPOT_LOG(error, "An error has occured in the stream %d", ret);
            retries--;
            CSPOT_LOG(error, "Retries left:%d", retries);
#ifndef CONFIG_BELL_NOCODEC
            ov_clear(&vorbisFile);
#endif
            size_t pos = this->currentTrackStream->getPosition();

            this->currentTrackStream = nullptr;
            this->currentTrackStream = track->getAudioFile();

            // Open the stream
#ifndef CONFIG_BELL_NOCODEC
            currentTrackStream->openStream();
            int32_t r =
                ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
#else
            ssize_t start_offset = 0;
            uint8_t* headerBuf = currentTrackStream->openStream(start_offset);
            if (start_offset < 0) {
              CSPOT_LOG(error, "Track failed to reload, skipping it");
              currentSongPlaying = false;
              properStream = false;
              eof = true;

            } else
#endif
            this->currentTrackStream->seek(pos);
          }
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
                  skipped = false;
                }
#endif
                written = dataCallback(pcmBuffer.data() + (ret - toWrite),
                                       toWrite, tracksPlayed, skipped);
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
      if (this->trackQueue->preloadedTracks.size() <= 1) {
        endOfQueueReached = true;
      }
#ifdef CONFIG_BELL_NOCODEC
      this->onTrackEnd(true);
#endif
    }
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
