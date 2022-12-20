#include "TrackPlayer.h"
#include <cstddef>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>
#include "CDNTrackStream.h"
#include "Logger.h"
#include "ivorbisfile.h"

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

TrackPlayer::TrackPlayer(std::shared_ptr<cspot::Context> ctx)
    : bell::Task("cspot_player", 32 * 1024, 5, 1) {
  this->ctx = ctx;
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

  std::cout << "Starging player task" << std::endl;
  startTask();
}

TrackPlayer::~TrackPlayer() {
  isRunning = false;
  std::scoped_lock lock(runningMutex);
}

void TrackPlayer::loadTrackFromRef(TrackRef* ref, size_t positionMs, bool startAutomatically) {
  this->playbackPosition = positionMs;
  this->autoStart = startAutomatically;

  this->playerStatus = Status::LOADING;
  auto nextTrack = trackProvider->loadFromTrackRef(ref);

  stopTrack();
  this->currentTrackStream = nextTrack;
  this->playbackSemaphore->give();
}

void TrackPlayer::stopTrack() {
  this->currentSongPlaying = false;
  std::scoped_lock lock(playbackMutex);
  this->currentTrackStream = nullptr;
}

void TrackPlayer::seekMs(size_t ms) {
  std::scoped_lock lock(seekMutex);
  ov_time_seek(&vorbisFile, ms);
}

void TrackPlayer::runTask() {
  std::scoped_lock lock(runningMutex);

  while (isRunning) {
    std::cout << "Track player waiting on playback semaphore" << std::endl;
    this->playbackSemaphore->twait(100);

    if (this->currentTrackStream == nullptr) {
      continue;
    }

    CSPOT_LOG(info, "Player received a track, waiting for it to be ready...");
    this->currentTrackStream->trackReady->wait();
    CSPOT_LOG(info, "Got track");

    if (this->currentTrackStream->status == CDNTrackStream::Status::FAILED) {
      CSPOT_LOG(error, "Track failed to load, aborting playback");
      this->playerStatus = Status::STOPPED;
      continue;
    }

    this->currentSongPlaying = true;

    this->trackLoaded();

    this->playbackMutex.lock();

    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);

    if (playbackPosition > 0) {
      ov_time_seek(&vorbisFile, playbackPosition);
    }

    bool eof = false;

    while (!eof && currentSongPlaying) {
      seekMutex.lock();
      long ret = ov_read(&vorbisFile, (char*)&pcmBuffer[0], pcmBuffer.size(), &currentSection);
      seekMutex.unlock();
      if (ret == 0) {
        CSPOT_LOG(info, "EOL");
        // and done :)
        eof = true;
      } else if (ret < 0) {
        CSPOT_LOG(error, "An error has occured in the stream %d", ret);
        eof = true;
      } else {
        if (this->dataCallback != nullptr) {
          dataCallback(pcmBuffer.data(), ret);
        }
      }
    }

    this->playbackMutex.unlock();
  }
}

size_t TrackPlayer::_vorbisRead(void* ptr, size_t size, size_t nmemb) {
  return this->currentTrackStream->readBytes((uint8_t*)ptr, nmemb * size);
}

size_t TrackPlayer::_vorbisClose() {
  return 0;
}

void TrackPlayer::setTrackLoadedCallback(TrackLoadedCallback callback) {
  this->trackLoaded = callback;
}

int TrackPlayer::_vorbisSeek(int64_t offset, int whence) {
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
  return this->currentTrackStream->getPosition();
}

CDNTrackStream::TrackInfo TrackPlayer::getCurrentTrackInfo() {
  return this->currentTrackStream->trackInfo;
}

void TrackPlayer::setDataCallback(DataCallback callback) {
  this->dataCallback = callback;
}