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
}

void SpircHandler::subscribeToMercury() {
  auto responseLambda = [=](MercurySession::Response& res) {
    sendCmd(MessageType_kMessageTypeHello);
    CSPOT_LOG(debug, "Sent kMessageTypeHello!");
  };
  auto subscriptionLambda = [=](MercurySession::Response& res) {
    CSPOT_LOG(debug, "Received subscription response");
    this->handleFrame(res.parts[0]);
  };

  ctx->session->executeSubscription(MercurySession::RequestType::SUB,
                                    "hm://remote/user/fliperspotify/",
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
      // sendEvent(CSpotEventType::SEEK, (int)state->remoteFrame.position);
      // state->updatePositionMs(state->remoteFrame.position);
      // this->player->seekMs(state->remoteFrame.position);
      // notify();
      break;
    }
    case MessageType_kMessageTypeVolume:
      // sendEvent(CSpotEventType::VOLUME, (int)state->remoteFrame.volume);
      // setVolume(state->remoteFrame.volume);
      break;
    case MessageType_kMessageTypePause:
      // setPause(true);
      break;
    case MessageType_kMessageTypePlay:
      // setPause(false);
      break;
    case MessageType_kMessageTypeNext:
      // sendEvent(CSpotEventType::NEXT);
      // nextSong();
      break;
    case MessageType_kMessageTypePrev:
      // sendEvent(CSpotEventType::PREV);
      // prevSong();
      break;
    case MessageType_kMessageTypeLoad: {
      CSPOT_LOG(debug, "Load frame!");

      playbackState.setActive(true);

      // Every sane person on the planet would expect std::move to work here.
      // And it does... on every single platform EXCEPT for ESP32 for some
      // reason. For which it corrupts memory and makes printf fail. so yeah.
      // its cursed.
      playbackState.updateTracks();

      // bool isPaused = (state->remoteFrame.state->status.value() ==
      // PlayStatus::kPlayStatusPlay) ? false : true;
      // loadTrack(player->hasTrack(), state->remoteFrame.state.position_ms,
      // false); state->updatePositionMs(state->remoteFrame.state.position_ms);

      this->notify();
      this->trackPlayer->loadTrackFromRed(playbackState.getCurrentTrack());
      break;
    }
    case MessageType_kMessageTypeReplace: {
      CSPOT_LOG(debug, "Got replace frame");
      // state->updateTracks();
      // this->notify();
      break;
    }
    case MessageType_kMessageTypeShuffle: {
      CSPOT_LOG(debug, "Got shuffle frame");
      // state->setShuffle(state->remoteFrame.state.shuffle);
      // this->notify();
      break;
    }
    case MessageType_kMessageTypeRepeat: {
      CSPOT_LOG(debug, "Got repeat frame");
      // state->setRepeat(state->remoteFrame.state.repeat);
      // this->notify();
      break;
    }
    default:
      break;
  }
}

void SpircHandler::notify() {
  this->sendCmd(MessageType_kMessageTypeNotify);
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