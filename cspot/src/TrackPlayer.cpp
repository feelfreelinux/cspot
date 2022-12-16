#include "TrackPlayer.h"
#include <fstream>
#include <memory>
#include "Logger.h"

using namespace cspot;

static size_t vorbisReadCb(void *ptr, size_t size, size_t nmemb, TrackPlayer *self)
{
    return 0;
}
static int vorbisCloseCb(TrackPlayer *self)
{
    return 0;
}

static int vorbisSeekCb(TrackPlayer *self, int64_t offset, int whence)
{

    return 0;
}

static long vorbisTellCb(TrackPlayer *self)
{
    return 0;
}

TrackPlayer::TrackPlayer(std::shared_ptr<cspot::Context> ctx)
    : bell::Task("cspot_player", 4 * 1024, 0, 0) {
  this->ctx = ctx;
  this->trackProvider = std::make_shared<cspot::TrackProvider>(ctx);
  this->playbackSemaphore = std::make_unique<bell::WrappedSemaphore>(0);

  startTask();
}

TrackPlayer::~TrackPlayer() {}

void TrackPlayer::loadTrackFromRed(TrackRef* ref) {
  this->playerStatus = Status::LOADING;
  this->currentTrackStream = trackProvider->loadFromTrackRef(ref);
  this->playbackSemaphore->give();
}

void TrackPlayer::runTask() {
  std::fstream file("dupa.opus", std::ios::binary | std::ios::app);
  while (true) {
    this->playbackSemaphore->wait();

    CSPOT_LOG(info, "Player received a track, waiting for it to be ready...");
    this->currentTrackStream->trackReady->wait();

    if (this->currentTrackStream->status == CDNTrackStream::Status::FAILED) {
      CSPOT_LOG(error, "Track failed to load, aborting playback");
      this->playerStatus = Status::STOPPED;
      continue;
    }

    CSPOT_LOG(info, "Track is ready, starting playback");
    this->playerStatus = Status::PLAYING;

    while (true) {
      std::vector<uint8_t> buffer(1024 * 2);
      size_t bytesRead = this->currentTrackStream->readBytes(buffer.data(), 1024);
      file.write((char*)buffer.data(), bytesRead);
    }
  }
}