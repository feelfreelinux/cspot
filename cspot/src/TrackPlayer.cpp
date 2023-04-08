#include "TrackPlayer.h"
#include <cstddef>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>
#include "CDNTrackStream.h"
#include "Logger.h"
#include "TrackReference.h"

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

TrackPlayer::TrackPlayer(std::shared_ptr<cspot::Context> ctx, isAiringCallback isAiring, EOFCallback eof, TrackLoadedCallback trackLoaded)
    : bell::Task("cspot_player", 48 * 1024, 5, 1) {
  this->ctx = ctx;
  this->isAiring = isAiring;
  this->eofCallback = eof;
  this->trackLoaded = trackLoaded;
  this->trackProvider = std::make_shared<cspot::TrackProvider>(ctx);
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
                                   bool startAutomatically) {
  this->playbackPosition = positionMs;
  this->autoStart = startAutomatically;

  auto nextTrack = trackProvider->loadFromTrackRef(ref);

  stopTrack();
  this->sequence++;
  this->currentTrackStream = nextTrack;
  this->playbackSemaphore->give();
}

void TrackPlayer::stopTrack() {
  this->currentSongPlaying = false;
  std::scoped_lock lock(playbackMutex);
}

void TrackPlayer::seekMs(size_t ms) {
  std::scoped_lock lock(seekMutex);
#ifdef BELL_VORBIS_FLOAT
  ov_time_seek(&vorbisFile, (double)ms / 1000);
#else
  ov_time_seek(&vorbisFile, ms);
#endif
}

void TrackPlayer::runTask() {
  std::scoped_lock lock(runningMutex);

  while (isRunning) {
    this->playbackSemaphore->twait(100);

    if (this->currentTrackStream == nullptr) {
      continue;
    }

    CSPOT_LOG(info, "Player received a track, waiting for it to be ready...");
    
    // when track changed many times and very quickly, we are stuck on never-given semaphore
    while (this->currentTrackStream->trackReady->twait(250));
    CSPOT_LOG(info, "Got track");

    if (this->currentTrackStream->status == CDNTrackStream::Status::FAILED) {
      CSPOT_LOG(error, "Track failed to load, skipping it");
      this->currentTrackStream = nullptr;
      this->eofCallback();
      continue;
    }

    this->currentSongPlaying = true;

    this->trackLoaded();

    this->playbackMutex.lock();

    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);

    if (playbackPosition > 0) {
#ifdef BELL_VORBIS_FLOAT
      ov_time_seek(&vorbisFile, (double)playbackPosition / 1000);
#else
      ov_time_seek(&vorbisFile, playbackPosition);
#endif
    }

    bool eof = false;

    while (!eof && currentSongPlaying) {
      seekMutex.lock();
#ifdef BELL_VORBIS_FLOAT
      long ret = ov_read(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(), 
                         0, 2, 1, &currentSection);
#else
      long ret = ov_read(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(),
                         &currentSection);
#endif
      seekMutex.unlock();
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
            auto written =
                dataCallback(pcmBuffer.data() + (ret - toWrite), toWrite,
                             this->currentTrackStream->trackInfo.trackId, this->sequence);
            if (written == 0) {
              BELL_SLEEP_MS(50);
            }
            toWrite -= written;
          }
        }
      }
    }
    ov_clear(&vorbisFile);

    // With very large buffers, track N+1 can be downloaded while N has not aired yet and
    // if we continue, the currentTrackStream will be emptied, causing a crash in
    // notifyAudioReachedPlayback when it will look for trackInfo. A busy loop is never 
    // ideal, but this low impact, infrequent and more simple than yet another semaphore
    while (currentSongPlaying && !isAiring()) {
        BELL_SLEEP_MS(100);
    }

    // always move back to LOADING (ensure proper seeking after last track has been loaded)
    this->currentTrackStream.reset();
    this->playbackMutex.unlock();

    if (eof) {
      this->eofCallback();
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

CDNTrackStream::TrackInfo TrackPlayer::getCurrentTrackInfo() {
  return this->currentTrackStream->trackInfo;
}

void TrackPlayer::setDataCallback(DataCallback callback) {
  this->dataCallback = callback;
}
