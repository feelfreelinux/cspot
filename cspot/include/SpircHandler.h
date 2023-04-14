#pragma once

#include <stdint.h>    // for uint32_t, uint8_t
#include <functional>  // for function
#include <memory>      // for shared_ptr, unique_ptr
#include <string>      // for string
#include <variant>     // for variant
#include <vector>      // for vector

#include "CDNTrackStream.h"     // for CDNTrackStream, CDNTrackStream::Track...
#include "PlaybackState.h"      // for PlaybackState
#include "protobuf/spirc.pb.h"  // for MessageType

namespace cspot {
class TrackPlayer;
struct Context;

class SpircHandler {
 public:
  SpircHandler(std::shared_ptr<cspot::Context> ctx);

  enum class EventType {
    PLAY_PAUSE,
    VOLUME,
    TRACK_INFO,
    DISC,
    NEXT,
    PREV,
    SEEK,
    DEPLETED,
    FLUSH,
    PLAYBACK_START
  };
  typedef std::variant<CDNTrackStream::TrackInfo, int, bool> EventData;

  struct Event {
    EventType eventType;
    EventData data;
  };

  typedef std::function<void(std::unique_ptr<Event>)> EventHandler;

  void subscribeToMercury();
  std::shared_ptr<TrackPlayer> getTrackPlayer();

  void setEventHandler(EventHandler handler);

  void setPause(bool pause);

  void nextSong();
  void previousSong();

  void notifyAudioReachedPlayback();
  void updatePositionMs(uint32_t position);
  void setRemoteVolume(int volume);
  void loadTrackFromURI(const std::string& uri);

  void disconnect();

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::TrackPlayer> trackPlayer;

  EventHandler eventHandler = nullptr;

  cspot::PlaybackState playbackState;
  CDNTrackStream::TrackInfo currentTrackInfo;

  bool isTrackFresh = true;
  bool isRequestedFromLoad = false;
  bool isNextTrackPreloaded = false;
  uint32_t nextTrackPosition = 0;

  void sendCmd(MessageType typ);

  void sendEvent(EventType type);
  void sendEvent(EventType type, EventData data);

  void handleFrame(std::vector<uint8_t>& data);
  void notify();
};
}  // namespace cspot