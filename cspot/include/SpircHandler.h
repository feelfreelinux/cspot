#pragma once

#include <memory>
#include "BellTask.h"

#include "CDNTrackStream.h"
#include "CSpotContext.h"
#include "PlaybackState.h"
#include "TrackPlayer.h"
#include "TrackProvider.h"
#include "protobuf/spirc.pb.h"

namespace cspot {
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
    LOAD,
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
  void setRemoteVolume(int volume);

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::TrackPlayer> trackPlayer;

  EventHandler eventHandler = nullptr;

  cspot::PlaybackState playbackState;
  void sendCmd(MessageType typ);

  void sendEvent(EventType type);
  void sendEvent(EventType type, EventData data);

  void handleFrame(std::vector<uint8_t>& data);
  void notify();
};
}  // namespace cspot