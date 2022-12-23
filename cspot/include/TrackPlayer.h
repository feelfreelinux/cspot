#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <BellUtils.h>
#include <WrappedSemaphore.h>
#include "CDNTrackStream.h"
#include "CSpotContext.h"
#include "TrackProvider.h"
#include "ivorbisfile.h"

namespace cspot {
class TrackPlayer : bell::Task {
 public:
  TrackPlayer(std::shared_ptr<cspot::Context> ctx);
  ~TrackPlayer();

  typedef std::function<void()> TrackLoadedCallback;
  typedef std::function<int(uint8_t*, size_t, std::string_view)> DataCallback;
  typedef std::function<void()> EOFCallback;

  enum class Status { STOPPED, LOADING, PLAYING, PAUSED };
  Status playerStatus;

  void loadTrackFromRef(TrackRef* ref, size_t playbackMs, bool startAutomatically);
  void setTrackLoadedCallback(TrackLoadedCallback callback);
  void setEOFCallback(EOFCallback callback);
  void setDataCallback(DataCallback callback);

  CDNTrackStream::TrackInfo getCurrentTrackInfo();
  void seekMs(size_t ms);
  void stopTrack();

  // Vorbis codec callbacks
  size_t _vorbisRead(void* ptr, size_t size, size_t nmemb);
  size_t _vorbisClose();
  int _vorbisSeek(int64_t offset, int whence);
  long _vorbisTell();

  void destroy();

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::TrackProvider> trackProvider;
  std::shared_ptr<cspot::CDNTrackStream> currentTrackStream;

  std::unique_ptr<bell::WrappedSemaphore> playbackSemaphore;

  TrackLoadedCallback trackLoaded;
  DataCallback dataCallback = nullptr;
  EOFCallback eofCallback = nullptr;

  // Playback control
  std::atomic<bool> currentSongPlaying;
  std::mutex playbackMutex;
  std::mutex seekMutex;

  // Vorbis related
  OggVorbis_File vorbisFile;
  ov_callbacks vorbisCallbacks;
  int currentSection;
  std::vector<uint8_t> pcmBuffer = std::vector<uint8_t>(1024);

  size_t playbackPosition = 0;
  bool autoStart = false;
  std::atomic<bool> isRunning = true;
  std::mutex runningMutex;

  void runTask() override;
};
}  // namespace cspot
