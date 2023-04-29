#pragma once

#include <stddef.h>  // for size_t
#include <deque>     // for deque
#include <memory>    // for shared_ptr
#include <mutex>     // for mutex
#include <variant>   // for variant
#include <vector>    // for vector
#include <atomic>

#include "BellTask.h"
#include "PlaybackState.h"
#include "TrackReference.h"

#include "protobuf/metadata.pb.h"  // for Track, _Track, AudioFile, Episode

namespace bell {
struct WrappedSemaphore;
};

namespace cspot {
struct Context;
struct AccessKeyFetcher;
struct CDNAudioFile;
class QueuedTrack {
 public:
  QueuedTrack(TrackReference& ref, std::shared_ptr<cspot::Context> ctx,
              uint32_t requestedPosition = 0);
  ~QueuedTrack();

  enum class State {
    QUEUED,
    PENDING_META,
    KEY_REQUIRED,
    PENDING_KEY,
    CDN_REQUIRED,
    READY,
    FAILED
  };

  State state;

  TrackReference ref;

  std::shared_ptr<bell::WrappedSemaphore> loadedSemaphore;

  uint32_t requestedPosition;

  std::string identifier;

  // Will return nullptr if the track is not ready
  std::shared_ptr<cspot::CDNAudioFile> getAudioFile();

  // --- Steps ---
  void stepLoadMetadata(
      Track* pbTrack, Episode* pbEpisode, std::mutex& trackListMutex,
      std::shared_ptr<bell::WrappedSemaphore> updateSemaphore);

  void stepParseMetadata(Track* pbTrack, Episode* pbEpisode);

  void stepLoadAudioFile(
      std::mutex& trackListMutex,
      std::shared_ptr<bell::WrappedSemaphore> updateSemaphore);

  void stepLoadCDNUrl(const std::string& accessKey);

  void expire();

 private:
  std::shared_ptr<cspot::Context> ctx;

  uint64_t pendingMercuryRequest = 0;
  uint32_t pendingAudioKeyRequest = 0;

  std::vector<uint8_t> trackId, fileId, audioKey;
  std::string cdnUrl;
};

class TrackQueue : public bell::Task {
 public:
  TrackQueue(std::shared_ptr<cspot::Context> ctx,
             std::shared_ptr<cspot::PlaybackState> playbackState);
  ~TrackQueue();

  enum class PlaybackEvent {
    FIRST_LOADED,
  };

  enum class SkipDirection { NEXT, PREV };

  using PlaybackEventData = std::variant<bool>;
  using PlaybackEventHandler =
      std::function<void(PlaybackEvent, std::shared_ptr<QueuedTrack>)>;

  std::shared_ptr<bell::WrappedSemaphore> playableSemaphore;

  PlaybackEventHandler onPlaybackEvent = nullptr;

  std::atomic<bool> notifyPending = false;

  void updateTracks(uint32_t requestedPosition = 0, bool initial = false);
  int getTrackRelativePosition(std::shared_ptr<QueuedTrack> track);

  void runTask() override;

  void stopTask();

  bool hasTracks();

  bool isFinished();

  bool skipTrack(SkipDirection dir, bool expectNotify = true);

  std::shared_ptr<QueuedTrack> consumeTrack(
      std::shared_ptr<QueuedTrack> prevSong, int& offset);

 private:
  static const int MAX_TRACKS_PRELOAD = 2;

  std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher;
  std::shared_ptr<PlaybackState> playbackState;
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<bell::WrappedSemaphore> processSemaphore;

  std::deque<std::shared_ptr<QueuedTrack>> preloadedTracks;

  std::vector<TrackReference> currentTracks;

  std::mutex tracksMutex, runningMutex;

  // PB data
  Track pbTrack;
  Episode pbEpisode;

  std::string accessKey;

  int16_t currentTracksIndex = -1;

  bool isRunning = false;

  void processTrack(std::shared_ptr<QueuedTrack> track);

  bool queueNextTrack(int offset = 0, uint32_t positionMs = 0);

  void syncWithState();
};
}  // namespace cspot
