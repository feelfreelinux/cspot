#pragma once

#include <atomic>            // for atomic
#include <cstdint>           // for uint8_t, int64_t
#include <ctime>             // for size_t, time
#include <functional>        // for function
#include <memory>            // for shared_ptr, unique_ptr
#include <mutex>             // for mutex
#include <string_view>       // for string_view
#include <vector>            // for vector

#include "BellTask.h"        // for Task
#include "CDNTrackStream.h"  // for CDNTrackStream, CDNTrackStream::TrackInfo

namespace bell {
class WrappedSemaphore;
}  // namespace bell
#ifdef BELL_VORBIS_FLOAT
#include "vorbis/vorbisfile.h"
#else
#include "ivorbisfile.h"     // for OggVorbis_File, ov_callbacks
#endif

namespace cspot {
class TrackProvider;
struct Context;
struct TrackReference;

class TrackPlayer : bell::Task {
 public:
  typedef std::function<void()> TrackLoadedCallback;
  typedef std::function<size_t(uint8_t*, size_t, std::string_view, size_t)> DataCallback;
  typedef std::function<void()> EOFCallback;
  typedef std::function<bool()> isAiringCallback;

  TrackPlayer(std::shared_ptr<cspot::Context> ctx, isAiringCallback, EOFCallback, TrackLoadedCallback);
  ~TrackPlayer();
      
  void loadTrackFromRef(TrackReference& ref, size_t playbackMs, bool startAutomatically);
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
  size_t sequence = std::time(nullptr);

  std::unique_ptr<bell::WrappedSemaphore> playbackSemaphore;

  TrackLoadedCallback trackLoaded;
  DataCallback dataCallback = nullptr;
  EOFCallback eofCallback;
  isAiringCallback isAiring;

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
