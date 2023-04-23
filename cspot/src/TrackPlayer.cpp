#include "TrackPlayer.h"

#include <mutex>        // for mutex, scoped_lock
#include <string>       // for string
#include <type_traits>  // for remove_extent_t
#include <vector>       // for vector, vector<>::value_type

#include "BellLogger.h"        // for AbstractLogger
#include "BellUtils.h"         // for BELL_SLEEP_MS
#include "Logger.h"            // for CSPOT_LOG
#include "Packet.h"            // for cspot
#include "TrackQueue.h"        // for CDNTrackStream, CDNTrackStream::TrackInfo
#include "WrappedSemaphore.h"  // for WrappedSemaphore

namespace cspot {
struct Context;
struct TrackReference;
}  // namespace cspot

using namespace cspot;

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

TrackPlayer::TrackPlayer(std::shared_ptr<cspot::Context> ctx,
                         std::shared_ptr<cspot::TrackQueue> trackQueue,
                         isAiringCallback isAiring, EOFCallback eof,
                         TrackLoadedCallback trackLoaded)
    : bell::Task("cspot_player", 48 * 1024, 5, 1) {
  this->ctx = ctx;
  this->isAiring = isAiring;
  this->eofCallback = eof;
  this->trackLoaded = trackLoaded;
  this->trackQueue = trackQueue;
  this->playbackSemaphore = std::make_unique<bell::WrappedSemaphore>(5);

  // Initialize vorbis callbacks
  vorbisFile = {};
  vorbisCallbacks = {
      (decltype(ov_callbacks::read_func))&vorbisReadCb,
      (decltype(ov_callbacks::seek_func))&vorbisSeekCb,
      (decltype(ov_callbacks::close_func))&vorbisCloseCb,
      (decltype(ov_callbacks::tell_func))&vorbisTellCb,
  };
  isRunning = true;

  startTask();
}

TrackPlayer::~TrackPlayer() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}

void TrackPlayer::loadTrackFromRef(TrackReference& ref, size_t positionMs,
                                   bool startAutomatically) {}

void TrackPlayer::resetState() {
  this->trackOffset = -1;
  this->currentSongPlaying = false;
  CSPOT_LOG(info, "Resetting state");
}

void TrackPlayer::seekMs(size_t ms) {

  CSPOT_LOG(info, "Seeking, track offset %d", trackOffset.load());
  if (trackOffset > 0) {
    // We're in the middle of the next track, so we need to reset the player in order to seek
    resetState();
  }

  this->pendingSeekPositionMs = ms;
}

void TrackPlayer::runTask() {
  std::scoped_lock lock(runningMutex);

  while (isRunning) {
    // Ensure we even have any tracks to play
    if (!this->trackQueue->hasTracks()) {
      this->trackQueue->playableSemaphore->twait(100);
      continue;
    }

    // Last track was interrupted, reset to default
    if (trackOffset == -1) {
      trackOffset = 0;
    }

    auto track = this->trackQueue->consumeTrack(trackOffset);

    if (track == nullptr) {
      BELL_SLEEP_MS(50);
      continue;
    }

    if (track->state != QueuedTrack::State::READY) {
      CSPOT_LOG(info, "Player received a track, waiting for it to be ready...");
      track->loadedSemaphore->twait(5000);

      if (track->state != QueuedTrack::State::READY) {
        CSPOT_LOG(error, "Track failed to load, skipping it");
        this->eofCallback();
        continue;
      }
    }

    CSPOT_LOG(info, "Got track");
    this->currentSongPlaying = true;
    this->trackLoaded();

    this->playbackMutex.lock();

    currentTrackStream = track->getAudioFile();

    // Open the stream
    currentTrackStream->openStream();

    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);

    if (pendingSeekPositionMs > 0) {
      track->requestedPosition = pendingSeekPositionMs;
    }

    if (track->requestedPosition > 0) {
#ifdef BELL_VORBIS_FLOAT
      ov_time_seek(&vorbisFile, (double)playbackPosition / 1000);
#else
      ov_time_seek(&vorbisFile, playbackPosition);
#endif
    }

    bool eof = false;

    while (!eof && currentSongPlaying) {
      // Execute seek if needed
      if (pendingSeekPositionMs > 0) {
        uint32_t seekPosition = pendingSeekPositionMs;

        // Reset the pending seek position
        pendingSeekPositionMs = 0;

        // Seek to the new position
#ifdef BELL_VORBIS_FLOAT
        ov_time_seek(&vorbisFile, (double)seekPosition / 1000);
#else
        ov_time_seek(&vorbisFile, seekPosition);
#endif
      }
#ifdef BELL_VORBIS_FLOAT
      long ret = ov_read(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(), 0,
                         2, 1, &currentSection);
#else
      long ret = ov_read(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(),
                         &currentSection);
#endif

      if (ret == 0) {
        CSPOT_LOG(info, "EOF");
        // and done :)
        eof = true;
      } else if (ret < 0) {
        CSPOT_LOG(error, "An error has occured in the stream %d", ret);
        currentSongPlaying = false;
      } else {
        if (this->dataCallback != nullptr) {
          auto toWrite = ret;

          while (!eof && currentSongPlaying && toWrite > 0) {
            auto written = dataCallback(pcmBuffer.data() + (ret - toWrite),
                                        toWrite, "", this->sequence);
            if (written == 0) {
              BELL_SLEEP_MS(50);
            }
            toWrite -= written;
          }
        }
      }
    }
    ov_clear(&vorbisFile);

    // // With very large buffers, track N+1 can be downloaded while N has not aired yet and
    // // if we continue, the currentTrackStream will be emptied, causing a crash in
    // // notifyAudioReachedPlayback when it will look for trackInfo. A busy loop is never
    // // ideal, but this low impact, infrequent and more simple than yet another semaphore
    // while (currentSongPlaying && !isAiring()) {
    //   BELL_SLEEP_MS(100);
    // }

    // always move back to LOADING (ensure proper seeking after last track has been loaded)
    this->currentTrackStream.reset();
    this->playbackMutex.unlock();

    if (eof) {
      this->eofCallback();
    }

    // Mark offset as ahead
    if (trackOffset != -1) {
      trackOffset = trackQueue->getTrackRelativePosition(track) + 1;
    }
  }
}

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

void TrackPlayer::setDataCallback(DataCallback callback) {
  this->dataCallback = callback;
}
