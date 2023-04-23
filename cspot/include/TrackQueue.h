#pragma once

#include <stddef.h>  // for size_t
#include <memory>    // for shared_ptr
#include <mutex>     // for mutex
#include <vector>    // for vector

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
  QueuedTrack(TrackRef* ref, std::shared_ptr<cspot::Context> ctx,
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

  std::shared_ptr<bell::WrappedSemaphore> loadedSemaphore;

  uint32_t requestedPosition;

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
  TrackReference ref;

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

  typedef std::variant<bool> PlaybackEventData;

  typedef std::function<void(PlaybackEvent, std::shared_ptr<QueuedTrack>)>
      PlaybackEventHandler;

  std::shared_ptr<bell::WrappedSemaphore> playableSemaphore;

  PlaybackEventHandler onPlaybackEvent = nullptr;

  void updateTracks(uint32_t requestedPosition = 0);
  int getTrackRelativePosition(std::shared_ptr<QueuedTrack> track);

  void runTask() override;
  void stopTask();

  bool hasTracks();

  std::shared_ptr<QueuedTrack> consumeTrack(int offset);

 private:
  static const int MAX_TRACKS_PRELOAD = 2;

  std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher;
  std::shared_ptr<PlaybackState> playbackState;
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<bell::WrappedSemaphore> processSemaphore;

  std::vector<std::shared_ptr<QueuedTrack>> tracks;
  std::mutex tracksMutex, runningMutex;

  // PB data
  Track pbTrack;
  Episode pbEpisode;

  std::string accessKey;

  int16_t currentTracksIndex = -1;

  bool isRunning = false;

  void processTrack(std::shared_ptr<QueuedTrack> track);
};
}  // namespace cspot