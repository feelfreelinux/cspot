#include "SpircHandler.h"
#include <memory>
#include "AccessKeyFetcher.h"
#include "BellUtils.h"
#include "CSpotContext.h"
#include "Logger.h"
#include "MercurySession.h"
#include "PlaybackState.h"
#include "TrackPlayer.h"

using namespace cspot;

SpircHandler::SpircHandler(std::shared_ptr<cspot::Context> ctx)
    : playbackState(ctx) {
  this->ctx = ctx;
  this->trackPlayer = std::make_shared<TrackPlayer>(ctx);

  this->trackPlayer->setTrackLoadedCallback([this]() {
    playbackState.setPlaybackState(PlaybackState::State::Playing);
    this->notify();

    setPause(false);
    sendEvent(EventType::PLAYBACK_START);
    sendEvent(EventType::TRACK_INFO, this->trackPlayer->getCurrentTrackInfo());
  });
}

void SpircHandler::subscribeToMercury() {
  auto responseLambda = [=](MercurySession::Response& res) {
    sendCmd(MessageType_kMessageTypeHello);
    CSPOT_LOG(debug, "Sent kMessageTypeHello!");

    // Assign country code
    this->ctx->config.countryCode = this->ctx->session->getCountryCode();
  };
  auto subscriptionLambda = [=](MercurySession::Response& res) {
    CSPOT_LOG(debug, "Received subscription response");
    this->handleFrame(res.parts[0]);
  };

  ctx->session->executeSubscription(MercurySession::RequestType::SUB,
                                    "hm://remote/user/" + ctx->config.username + "/",
                                    responseLambda, subscriptionLambda);
}

void SpircHandler::handleFrame(std::vector<uint8_t>& data) {
  pb_release(Frame_fields, &playbackState.remoteFrame);
  pbDecode(playbackState.remoteFrame, Frame_fields, data);

  switch (playbackState.remoteFrame.typ) {
    case MessageType_kMessageTypeNotify: {
      CSPOT_LOG(debug, "Notify frame");
      // Pause the playback if another player took control
      // if (state->isActive() && state->remoteFrame.device_state.is_active) {
      //   disconnect();
      // }
      break;
    }
    case MessageType_kMessageTypeSeek: {
      CSPOT_LOG(debug, "Seek command");
      sendEvent(EventType::SEEK, (int)playbackState.remoteFrame.position);
      playbackState.updatePositionMs(playbackState.remoteFrame.position);
      trackPlayer->seekMs(playbackState.remoteFrame.position);
      notify();
      break;
    }
    case MessageType_kMessageTypeVolume:
      sendEvent(EventType::VOLUME, (int)playbackState.remoteFrame.volume);
      setRemoteVolume(playbackState.remoteFrame.volume);
      break;
    case MessageType_kMessageTypePause:
      setPause(true);
      break;
    case MessageType_kMessageTypePlay:
      setPause(false);
      break;
    case MessageType_kMessageTypeNext:
      sendEvent(EventType::NEXT);
      nextSong();
      break;
    case MessageType_kMessageTypePrev:
      sendEvent(EventType::PREV);
      previousSong();
      break;
    case MessageType_kMessageTypeLoad: {
      CSPOT_LOG(debug, "Load frame!");

      playbackState.setActive(true);
      playbackState.updateTracks();

      // loadTrack(player->hasTrack(), state->remoteFrame.state.position_ms,
      //           false);
      playbackState.updatePositionMs(
          playbackState.remoteFrame.state.position_ms);

      this->trackPlayer->loadTrackFromRef(
          playbackState.getCurrentTrack(),
          playbackState.remoteFrame.state.position_ms, true);
      playbackState.setPlaybackState(PlaybackState::State::Loading);
      playbackState.updatePositionMs(
          playbackState.remoteFrame.state.position_ms);

      this->notify();
      break;
    }
    case MessageType_kMessageTypeReplace: {
      CSPOT_LOG(debug, "Got replace frame");
      playbackState.updateTracks();
      this->notify();
      break;
    }
    case MessageType_kMessageTypeShuffle: {
      CSPOT_LOG(debug, "Got shuffle frame");
      playbackState.setShuffle(playbackState.remoteFrame.state.shuffle);
      this->notify();
      break;
    }
    case MessageType_kMessageTypeRepeat: {
      CSPOT_LOG(debug, "Got repeat frame");
      playbackState.setRepeat(playbackState.remoteFrame.state.repeat);
      this->notify();
      break;
    }
    default:
      break;
  }
}

void SpircHandler::setRemoteVolume(int volume) {
  playbackState.setVolume(volume);
}

void SpircHandler::notify() {
  this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircHandler::nextSong() {
  if (playbackState.nextTrack()) {
    this->trackPlayer->loadTrackFromRef(playbackState.getCurrentTrack(), 0,
                                        true);
  } else {
    // player->cancelCurrentTrack();
    trackPlayer->stopTrack();
  }
  notify();
}

void SpircHandler::previousSong() {
  playbackState.prevTrack();
  // loadTrack(true);
  notify();
}

std::shared_ptr<TrackPlayer> SpircHandler::getTrackPlayer() {
  return this->trackPlayer;
}

void SpircHandler::sendCmd(MessageType typ) {
  // Serialize current player state
  auto encodedFrame = playbackState.encodeCurrentFrame(typ);

  auto responseLambda = [=](MercurySession::Response& res) {
  };
  auto parts = MercurySession::DataParts({encodedFrame});
  ctx->session->execute(MercurySession::RequestType::SEND,
                        "hm://remote/user/" + ctx->config.username + "/",
                        responseLambda, parts);
}
void SpircHandler::setEventHandler(EventHandler handler) {
  this->eventHandler = handler;
}

void SpircHandler::setPause(bool isPaused) {
  sendEvent(EventType::PLAY_PAUSE, isPaused);
  if (isPaused) {
    CSPOT_LOG(debug, "External pause command");
    playbackState.setPlaybackState(PlaybackState::State::Paused);
  } else {
    CSPOT_LOG(debug, "External play command");

    playbackState.setPlaybackState(PlaybackState::State::Playing);
  }
  notify();
}

void SpircHandler::sendEvent(EventType type) {
  auto event = std::make_unique<Event>();
  event->eventType = type;
  event->data = {};
  eventHandler(std::move(event));
}

void SpircHandler::sendEvent(EventType type, EventData data) {
  auto event = std::make_unique<Event>();
  event->eventType = type;
  event->data = data;
  eventHandler(std::move(event));
}