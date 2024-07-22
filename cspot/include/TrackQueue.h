#pragma once

#include <stddef.h>  // for size_t
#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <random>   //for random_device and default_random_engine
#include <utility>  // for pair

#include "BellTask.h"
#include "EventManager.h"  // for TrackMetrics
#include "PlaybackState.h"
#include "PlayerContext.h"
#include "TrackReference.h"

#include "protobuf/metadata.pb.h"  // for Track, _Track, AudioFile, Episode

#ifndef CONFIG_UPDATE_FUTURE_TRACKS
#define CONFIG_UPDATE_FUTURE_TRACKS 10
#endif
#define inner_tracks_treshhold 10
#define SEND_OLD_TRACKS 2

namespace bell {
class WrappedSemaphore;
};

namespace cspot {
struct Context;
class AccessKeyFetcher;
class CDNAudioFile;

// Used in got track info event
struct TrackInfo {
  std::string name, album, artist, imageUrl, trackId;
  uint32_t duration, number, discNumber;

  void loadPbTrack(Track* pbTrack, const std::vector<uint8_t>& gid);
  void loadPbEpisode(Episode* pbEpisode, const std::vector<uint8_t>& gid);
};

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

  std::shared_ptr<bell::WrappedSemaphore> loadedSemaphore;

  State state = State::QUEUED;  // Current state of the track
  TrackReference ref;           // Holds GID, URI and Context
  TrackInfo trackInfo;  // Full track information fetched from spotify, name etc

  // PB data
  Track pbTrack = Track_init_zero;
  Episode pbEpisode = Episode_init_zero;
  uint32_t requestedPosition;
  uint64_t written_bytes = 0;
  std::string identifier;
  bool loading = false;
  std::shared_ptr<cspot::TrackMetrics> trackMetrics;
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

  enum class SkipDirection { NEXT, PREV };

  std::shared_ptr<bell::WrappedSemaphore> playableSemaphore;
  std::shared_ptr<PlaybackState> playbackState;
  std::atomic<bool> notifyPending = false;

  void runTask() override;
  void stopTask();

  void shuffle_tracks(bool shuffleTracks);
  void update_ghost_tracks(int16_t offset = 0);
  bool hasTracks();
  bool isFinished();
  void reloadTracks(uint8_t offset = 1);
  bool skipTrack(SkipDirection dir, bool expectNotify = true);
  bool updateTracks(uint32_t requestedPosition = 0, bool initial = false);
  TrackInfo getTrackInfo(std::string_view identifier);
  std::shared_ptr<QueuedTrack> consumeTrack(
      std::shared_ptr<QueuedTrack> prevSong, int& offset);

 private:
  static const int MAX_TRACKS_PRELOAD = 3;

  std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher;
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<bell::WrappedSemaphore> processSemaphore;
  std::unique_ptr<cspot::PlayerContext> playerContext;

  std::deque<std::shared_ptr<QueuedTrack>> preloadedTracks;
  std::deque<std::pair<int64_t, TrackReference>> queuedTracks = {};
  std::vector<TrackReference> currentTracks;
  std::vector<TrackReference> ghostTracks;
  std::mutex tracksMutex, runningMutex;

  std::string accessKey;
  uint32_t radio_offset = 0;

  uint32_t currentTracksIndex = -1;

  bool isRunning = false;
  bool context_resolved = false;
  bool continue_with_radio = true;

  std::random_device rd;
  std::default_random_engine rng;

  void resolveAutoplay();
  void resolveContext();
  void processTrack(std::shared_ptr<QueuedTrack> track);
  bool queueNextTrack(int offset = 0, uint32_t positionMs = 0);
  void loadRadio(std::string req);
};
}  // namespace cspot
