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

  this->trackPlayer->setEOFCallback([this]() {
    auto ref = this->playbackState.getNextTrackRef();

    if (!isNextTrackPreloaded && ref != nullptr) {
      isNextTrackPreloaded = true;
      this->trackPlayer->loadTrackFromRef(ref, 0, true);
    }
  });

  this->trackPlayer->setTrackLoadedCallback([this]() {
    this->currentTrackInfo = this->trackPlayer->getCurrentTrackInfo();

    if (isRequestedFromLoad) {
      sendEvent(EventType::PLAYBACK_START);
      setPause(false);
    }
  });

  // Subscribe to mercury on session ready
  ctx->session->setConnectedHandler([this]() { this->subscribeToMercury(); });
}

void SpircHandler::subscribeToMercury() {
  auto responseLambda = [=](MercurySession::Response& res) {
    if (res.fail) return;

    sendCmd(MessageType_kMessageTypeHello);
    CSPOT_LOG(debug, "Sent kMessageTypeHello!");

    // Assign country code
    this->ctx->config.countryCode = this->ctx->session->getCountryCode();
  };
  auto subscriptionLambda = [=](MercurySession::Response& res) {
    if (res.fail) return;
    CSPOT_LOG(debug, "Received subscription response");

    this->handleFrame(res.parts[0]);
  };

  ctx->session->executeSubscription(
      MercurySession::RequestType::SUB,
      "hm://remote/user/" + ctx->config.username + "/", responseLambda,
      subscriptionLambda);
}

void SpircHandler::notifyAudioReachedPlayback() {
  if (isRequestedFromLoad || isNextTrackPreloaded) {
    playbackState.updatePositionMs(nextTrackPosition);
    playbackState.setPlaybackState(PlaybackState::State::Playing);
  } else {
    setPause(true);
  }

  isRequestedFromLoad = false;

  if (isNextTrackPreloaded) {
    isNextTrackPreloaded = false;

    playbackState.nextTrack();
    nextTrackPosition = 0;
  }

  this->notify();

  sendEvent(EventType::TRACK_INFO, this->trackPlayer->getCurrentTrackInfo());
}

void SpircHandler::disconnect() {
  this->trackPlayer->stopTrack();
  this->ctx->session->disconnect();
}

void SpircHandler::handleFrame(std::vector<uint8_t>& data) {
  pb_release(Frame_fields, &playbackState.remoteFrame);
  pbDecode(playbackState.remoteFrame, Frame_fields, data);

  switch (playbackState.remoteFrame.typ) {
    case MessageType_kMessageTypeNotify: {
      CSPOT_LOG(debug, "Notify frame");

      // Pause the playback if another player took control
      if (playbackState.isActive() &&
          playbackState.remoteFrame.device_state.is_active) {
        CSPOT_LOG(debug, "Another player took control, pausing playback");
        playbackState.setActive(false);
        this->trackPlayer->stopTrack();
        sendEvent(EventType::DISC);

        disconnect();
      }
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
      playbackState.setVolume(playbackState.remoteFrame.volume);
      this->notify();
      sendEvent(EventType::VOLUME, (int)playbackState.remoteFrame.volume);
      break;
    case MessageType_kMessageTypePause:
      setPause(true);
      break;
    case MessageType_kMessageTypePlay:
      setPause(false);
      break;
    case MessageType_kMessageTypeNext:
      nextSong();
      sendEvent(EventType::NEXT);
      break;
    case MessageType_kMessageTypePrev:
      previousSong();
      sendEvent(EventType::PREV);
      break;
    case MessageType_kMessageTypeLoad: {
      CSPOT_LOG(debug, "Load frame!");
      isRequestedFromLoad = true;
      isNextTrackPreloaded = false;

      playbackState.setActive(true);
      playbackState.updateTracks();

      auto playbackRef = playbackState.getCurrentTrackRef();

      if (playbackRef != nullptr) {
        playbackState.updatePositionMs(
            playbackState.remoteFrame.state.position_ms);

        this->trackPlayer->loadTrackFromRef(
            playbackState.getCurrentTrackRef(),
            playbackState.remoteFrame.state.position_ms, true);
        playbackState.setPlaybackState(PlaybackState::State::Loading);
        this->nextTrackPosition = playbackState.remoteFrame.state.position_ms;
      }

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
  // notify();
}

void SpircHandler::notify() {
  this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircHandler::nextSong() {
  if (playbackState.nextTrack()) {
    isRequestedFromLoad = true;
    isNextTrackPreloaded = false;

    this->trackPlayer->loadTrackFromRef(playbackState.getCurrentTrackRef(), 0,
                                        true);

  } else {
    sendEvent(EventType::FLUSH);
    playbackState.updatePositionMs(0);
    trackPlayer->stopTrack();
  }
  this->nextTrackPosition = 0;
  notify();
}

void SpircHandler::previousSong() {
  playbackState.prevTrack();
  isRequestedFromLoad = true;
  isNextTrackPreloaded = false;

  sendEvent(EventType::PREV);
  this->trackPlayer->loadTrackFromRef(playbackState.getCurrentTrackRef(), 0,
                                      true);
  this->nextTrackPosition = 0;

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
  if (isPaused) {
    CSPOT_LOG(debug, "External pause command");
    playbackState.setPlaybackState(PlaybackState::State::Paused);
  } else {
    CSPOT_LOG(debug, "External play command");

    playbackState.setPlaybackState(PlaybackState::State::Playing);
  }
  notify();
  sendEvent(EventType::PLAY_PAUSE, isPaused);
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
