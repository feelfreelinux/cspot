#include "SpircHandler.h"

#include <cstdint>      // for uint8_t
#include <memory>       // for shared_ptr, make_unique, unique_ptr
#include <type_traits>  // for remove_extent_t
#include <utility>      // for move

#include "BellLogger.h"      // for AbstractLogger
#include "CSpotContext.h"    // for Context::ConfigState, Context (ptr only)
#include "Logger.h"          // for CSPOT_LOG
#include "MercurySession.h"  // for MercurySession, MercurySession::Response
#include "NanoPBHelper.h"    // for pbDecode
#include "Packet.h"          // for cspot
#include "PlaybackState.h"   // for PlaybackState, PlaybackState::State
#include "TrackPlayer.h"     // for TrackPlayer
#include "TrackQueue.h"
#include "TrackReference.h"     // for TrackReference
#include "Utils.h"              // for stringHexToBytes
#include "pb_decode.h"          // for pb_release
#include "protobuf/spirc.pb.h"  // for Frame, State, Frame_fields, MessageTy...

using namespace cspot;

SpircHandler::SpircHandler(std::shared_ptr<cspot::Context> ctx) {
  this->playbackState = std::make_shared<PlaybackState>(ctx);
  this->trackQueue = std::make_shared<cspot::TrackQueue>(ctx, playbackState);

  this->trackQueue->onPlaybackEvent = [this](
                                          TrackQueue::PlaybackEvent event,
                                          std::shared_ptr<QueuedTrack> track) {
    switch (event) {
      case TrackQueue::PlaybackEvent::FIRST_LOADED: {
        playbackState->setPlaybackState(PlaybackState::State::Playing);
        playbackState->updatePositionMs(track->requestedPosition);

        this->notify();
        sendEvent(EventType::PLAYBACK_START);
        break;
      }
    }
  };

  auto isAiringCallback = [this]() {
    // return !(isNextTrackPreloaded || isRequestedFromLoad);
    return false;
  };

  auto EOFCallback = [this]() {
    // auto ref = this->playbackState->getNextTrackRef();

    // if (!isNextTrackPreloaded && !isRequestedFromLoad && ref != nullptr) {
    //   isNextTrackPreloaded = true;
    //   auto trackRef = TrackReference::fromTrackRef(ref);
    //   this->trackPlayer->loadTrackFromRef(trackRef, 0, true);
    // }

    // if (ref == nullptr) {
    //   sendEvent(EventType::DEPLETED);
    // }
  };

  auto trackLoadedCallback = [this]() {
    // this->currentTrackInfo = this->trackPlayer->getCurrentTrackInfo();

    // if (isRequestedFromLoad) {
    //   sendEvent(EventType::PLAYBACK_START, (int)nextTrackPosition);
    //   setPause(false);
    // }
  };

  this->ctx = ctx;
  this->trackPlayer = std::make_shared<TrackPlayer>(
      ctx, trackQueue, isAiringCallback, EOFCallback, trackLoadedCallback);

  // Subscribe to mercury on session ready
  ctx->session->setConnectedHandler([this]() { this->subscribeToMercury(); });
}

void SpircHandler::subscribeToMercury() {
  auto responseLambda = [this](MercurySession::Response& res) {
    if (res.fail)
      return;

    sendCmd(MessageType_kMessageTypeHello);
    CSPOT_LOG(debug, "Sent kMessageTypeHello!");

    // Assign country code
    this->ctx->config.countryCode = this->ctx->session->getCountryCode();
  };
  auto subscriptionLambda = [this](MercurySession::Response& res) {
    if (res.fail)
      return;
    CSPOT_LOG(debug, "Received subscription response");

    this->handleFrame(res.parts[0]);
  };

  ctx->session->executeSubscription(
      MercurySession::RequestType::SUB,
      "hm://remote/user/" + ctx->config.username + "/", responseLambda,
      subscriptionLambda);
}

void SpircHandler::loadTrackFromURI(const std::string& uri) {
  // {track/episode}:{gid}
  bool isEpisode = uri.find("episode:") != std::string::npos;
  auto gid = stringHexToBytes(uri.substr(uri.find(":") + 1));
  auto trackRef = TrackReference::fromGID(gid, isEpisode);

  // isRequestedFromLoad = true;
  // isNextTrackPreloaded = false;

  playbackState->setActive(true);

  auto playbackRef = playbackState->getCurrentTrackRef();

  if (playbackRef != nullptr) {
    playbackState->updatePositionMs(
        playbackState->remoteFrame.state.position_ms);

    auto ref = TrackReference::fromTrackRef(playbackRef);
    this->trackPlayer->loadTrackFromRef(
        ref, playbackState->remoteFrame.state.position_ms, true);
    playbackState->setPlaybackState(PlaybackState::State::Loading);
    this->nextTrackPosition = playbackState->remoteFrame.state.position_ms;
  }

  this->notify();
}

void SpircHandler::notifyAudioReachedPlayback() {
  // if (isRequestedFromLoad || isNextTrackPreloaded) {
  //   playbackState->updatePositionMs(nextTrackPosition);
  //   playbackState->setPlaybackState(PlaybackState::State::Playing);
  // } else {
  //   setPause(true);
  // }

  // isRequestedFromLoad = false;

  // if (isNextTrackPreloaded) {
  //   isNextTrackPreloaded = false;

  //   playbackState->nextTrack();
  //   nextTrackPosition = 0;
  // }

  this->notify();

  // sendEvent(EventType::TRACK_INFO, this->trackPlayer->getCurrentTrackInfo());
}

void SpircHandler::updatePositionMs(uint32_t position) {
  playbackState->updatePositionMs(position);
  notify();
}

void SpircHandler::disconnect() {
  this->trackPlayer->resetState();
  this->ctx->session->disconnect();
}

void SpircHandler::handleFrame(std::vector<uint8_t>& data) {
  pb_release(Frame_fields, &playbackState->remoteFrame);
  pbDecode(playbackState->remoteFrame, Frame_fields, data);

  switch (playbackState->remoteFrame.typ) {
    case MessageType_kMessageTypeNotify: {
      CSPOT_LOG(debug, "Notify frame");

      // Pause the playback if another player took control
      if (playbackState->isActive() &&
          playbackState->remoteFrame.device_state.is_active) {
        CSPOT_LOG(debug, "Another player took control, pausing playback");
        playbackState->setActive(false);
        this->trackPlayer->resetState();
        sendEvent(EventType::DISC);
      }
      break;
    }
    case MessageType_kMessageTypeSeek: {
      /* If next track is already downloading, we can't seek in the current one anymore. Also,
       * when last track has been reached, we has to restart as we can't tell the difference */
      this->trackPlayer->seekMs(playbackState->remoteFrame.position);
      playbackState->updatePositionMs(playbackState->remoteFrame.position);
      notify();
      sendEvent(EventType::SEEK, (int)playbackState->remoteFrame.position);

      // if ((!isNextTrackPreloaded && this->playbackState->getNextTrackRef()) ||
      //     isRequestedFromLoad) {
      //   CSPOT_LOG(debug, "Seek command while streaming current");
      //   sendEvent(EventType::SEEK, (int)playbackState->remoteFrame.position);
      //   playbackState->updatePositionMs(playbackState->remoteFrame.position);
      //   trackPlayer->seekMs(playbackState->remoteFrame.position);
      // } else {
      //   CSPOT_LOG(debug, "Seek command while streaming next or before started");
      //   isRequestedFromLoad = true;
      //   isNextTrackPreloaded = false;
      //   auto ref =
      //       TrackReference::fromTrackRef(playbackState->getCurrentTrackRef());
      //   this->trackPlayer->loadTrackFromRef(
      //       ref, playbackState->remoteFrame.position, true);
      //   this->nextTrackPosition = playbackState->remoteFrame.position;
      //}
      break;
    }
    case MessageType_kMessageTypeVolume:
      playbackState->setVolume(playbackState->remoteFrame.volume);
      this->notify();
      sendEvent(EventType::VOLUME, (int)playbackState->remoteFrame.volume);
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
      // isRequestedFromLoad = true;
      // isNextTrackPreloaded = false;
      if (playbackState->remoteFrame.state.track_count <= 0) {
        CSPOT_LOG(info, "No tracks in frame, stopping playback");
        break;
      }

      playbackState->setActive(true);
      playbackState->updateTracks();
      playbackState->updatePositionMs(playbackState->remoteFrame.position);

      // Let the track player know that we're loading a new track
      playbackState->setPlaybackState(PlaybackState::State::Loading);

      // Stop the current track, if any
      trackPlayer->resetState();

      // Update track list in case we have a new one
      trackQueue->updateTracks(playbackState->remoteFrame.state.position_ms);

      this->notify();
      break;
    }
    case MessageType_kMessageTypeReplace: {
      CSPOT_LOG(debug, "Got replace frame");
      playbackState->updateTracks();
      this->notify();
      break;
    }
    case MessageType_kMessageTypeShuffle: {
      CSPOT_LOG(debug, "Got shuffle frame");
      playbackState->setShuffle(playbackState->remoteFrame.state.shuffle);
      this->notify();
      break;
    }
    case MessageType_kMessageTypeRepeat: {
      CSPOT_LOG(debug, "Got repeat frame");
      playbackState->setRepeat(playbackState->remoteFrame.state.repeat);
      this->notify();
      break;
    }
    default:
      break;
  }
}

void SpircHandler::setRemoteVolume(int volume) {
  playbackState->setVolume(volume);
  notify();
}

void SpircHandler::notify() {
  this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircHandler::nextSong() {
  // if (playbackState->nextTrack()) {
  //   // isRequestedFromLoad = true;
  //   // isNextTrackPreloaded = false;
  //   auto ref = TrackReference::fromTrackRef(playbackState->getCurrentTrackRef());
  //   this->trackPlayer->loadTrackFromRef(ref, 0, true);
  // } else {
  //   sendEvent(EventType::FLUSH);
  //   playbackState->updatePositionMs(0);
  //   trackPlayer->stopTrack();
  // }
  // this->nextTrackPosition = 0;
  notify();
}

void SpircHandler::previousSong() {
  playbackState->prevTrack();
  // isRequestedFromLoad = true;
  // isNextTrackPreloaded = false;

  sendEvent(EventType::PREV);
  auto ref = TrackReference::fromTrackRef(playbackState->getCurrentTrackRef());
  this->trackPlayer->loadTrackFromRef(ref, 0, true);
  this->nextTrackPosition = 0;

  notify();
}

std::shared_ptr<TrackPlayer> SpircHandler::getTrackPlayer() {
  return this->trackPlayer;
}

void SpircHandler::sendCmd(MessageType typ) {
  // Serialize current player state
  auto encodedFrame = playbackState->encodeCurrentFrame(typ);

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
    playbackState->setPlaybackState(PlaybackState::State::Paused);
  } else {
    CSPOT_LOG(debug, "External play command");

    playbackState->setPlaybackState(PlaybackState::State::Playing);
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
