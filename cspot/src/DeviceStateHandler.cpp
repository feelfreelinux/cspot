#include "DeviceStateHandler.h"

#include <string.h>  // for strdup, memcpy, strcpy, strlen
#include <cstdint>   // for uint8_t
#include <cstdlib>   // for unreference, NULL, realloc, rand
#include <cstring>
#include <memory>       // for shared_ptr
#include <type_traits>  // for remove_extent_t
#include <utility>      // for swap

#include "BellLogger.h"           // for AbstractLogger
#include "BellUtils.h"            // for BELL_SLEEP_MS
#include "CSpotContext.h"         // for Context::ConfigState, Context (ptr o...
#include "ConstantParameters.h"   // for protocolVersion, swVersion
#include "Logger.h"               // for CSPOT_LOG
#include "NanoPBHelper.h"         // for pbEncode, pbPutString
#include "Packet.h"               // for cspot
#include "TrackReference.h"       // for cspot
#include "WrappedSemaphore.h"     // for WrappedSemaphore
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#include "pb.h"                   // for pb_bytes_array_t, PB_BYTES_ARRAY_T_A...
#include "pb_decode.h"            // for pb_release

using namespace cspot;

#if defined(_WIN32) || defined(_WIN64)
char* strndup(const char* str, size_t n) {
  if (!str)
    return nullptr;  // Handle null input gracefully
  size_t len = std::strlen(str);
  if (len > n)
    len = n;                                 // Limit to n characters
  char* copy = (char*)std::malloc(len + 1);  // Allocate memory
  if (!copy)
    return nullptr;             // Return null if allocation fails
  std::memcpy(copy, str, len);  // Copy the characters
  copy[len] = '\0';             // Null-terminate the string
  return copy;
}
#endif
static DeviceStateHandler* handler;

void DeviceStateHandler::reloadTrackList(void* data) {
  if (data == NULL) {
    if (handler->reloadPreloadedTracks) {
      handler->needsToBeSkipped = true;
      while (!handler->trackQueue->playableSemaphore->twait(1)) {};
      handler->trackPlayer->start();
      handler->trackPlayer->resetState();
      handler->reloadPreloadedTracks = false;
      handler->sendCommand(CommandType::PLAYBACK_START);
      handler->device.player_state.track = handler->currentTracks[0];
    }
    if (!handler->offset) {
      if (handler->trackQueue->preloadedTracks.size())
        handler->trackQueue->preloadedTracks.clear();
      handler->trackQueue->preloadedTracks.push_back(
          std::make_shared<cspot::QueuedTrack>(
              handler->currentTracks[handler->offset], handler->ctx,
              handler->trackQueue->playableSemaphore,
              handler->offsetFromStartInMillis));
      handler->device.player_state.track =
          handler->currentTracks[handler->offset];
      handler->offsetFromStartInMillis = 0;
      handler->offset++;
    }
    if (!handler->trackQueue->preloadedTracks.size()) {
      handler->trackQueue->preloadedTracks.push_back(
          std::make_shared<cspot::QueuedTrack>(
              handler->currentTracks[handler->offset - 1], handler->ctx,
              handler->trackQueue->playableSemaphore,
              handler->offsetFromStartInMillis));
      handler->offsetFromStartInMillis = 0;
    }
    if (handler->currentTracks.size() >
        handler->trackQueue->preloadedTracks.size() + handler->offset) {
      while (handler->currentTracks.size() >
                 handler->trackQueue->preloadedTracks.size() +
                     handler->offset &&
             handler->trackQueue->preloadedTracks.size() < 3) {
        handler->trackQueue->preloadedTracks.push_back(
            std::make_shared<cspot::QueuedTrack>(
                handler->currentTracks
                    [handler->offset +
                     handler->trackQueue->preloadedTracks.size() - 1],
                handler->ctx, handler->trackQueue->playableSemaphore));
      }
    }
    if (handler->playerStateChanged) {
      handler->putPlayerState(
          PutStateReason::PutStateReason_PLAYER_STATE_CHANGED);
      handler->playerStateChanged = false;
    }
  }
  if (strcmp(handler->currentTracks[handler->offset - 1].uri,
             "spotify:delimiter") == 0 &&
      handler->device.player_state.is_playing &&
      handler->currentTracks.size() <= handler->offset) {
    handler->ctx->playbackMetrics->end_reason = cspot::PlaybackMetrics::REMOTE;
    handler->ctx->playbackMetrics->end_source = "unknown";
    handler->trackPlayer->stop();
    handler->device.player_state.has_is_playing = true;
    handler->device.player_state.is_playing = false;
    handler->device.player_state.track = ProvidedTrack_init_zero;
    handler->device.player_state.has_track = false;
    if (handler->device.player_state.has_restrictions)
      pb_release(Restrictions_fields,
                 &handler->device.player_state.restrictions);
    handler->device.player_state.restrictions = Restrictions_init_zero;
    handler->device.player_state.has_restrictions = false;
    handler->putPlayerState();
    handler->sendCommand(CommandType::DISC);
#ifndef CONFIG_CSPOT_STAY_CONNECTED_ON_TRANSFER
    handler->disconnect();
#endif
    return;
  }
  handler->resolvingContext.store(false);
  //CSPOT_LOG(info,"heap_memory_check-safe = %i",heap_caps_check_integrity_all(true));
}
DeviceStateHandler::DeviceStateHandler(std::shared_ptr<cspot::Context> ctx,
                                       std::function<void()> onClose) {
  handler = this;
  this->ctx = ctx;
  this->onClose = onClose;
  this->trackQueue = std::make_shared<cspot::TrackQueue>(ctx);
  this->playerContext = std::make_shared<cspot::PlayerContext>(
      ctx, &this->device.player_state, &currentTracks, &offset);

  auto onTrackEnd = [this](bool loaded) {
    if (!loaded) {}
    CSPOT_LOG(debug, "Ended track");
    if (needsToBeSkipped) {
      if (this->device.player_state.options.repeating_track)
        this->trackQueue->preloadedTracks[0]->requestedPosition = 0;
      else if (this->trackQueue->preloadedTracks.size())
        skip(CommandType::SKIP_NEXT, true);
    }
    this->device.player_state.timestamp =
        this->ctx->timeProvider->getSyncedTimestamp();
    needsToBeSkipped = true;
    if (!this->trackQueue->preloadedTracks.size())
      sendCommand(CommandType::DEPLETED);
    if ((uint32_t)currentTracks.size() / 2 <= offset &&
        !this->resolvingContext) {
      this->resolvingContext.store(true);
      playerContext->resolveTracklist(metadata_map, reloadTrackList);
    }
  };

  auto onTrackChanged = [this](std::shared_ptr<QueuedTrack> track,
                               bool new_track = false) {
    if (new_track) {
      this->device.player_state.timestamp =
          this->trackQueue->preloadedTracks[0]
              ->trackMetrics->currentInterval->start;

      this->device.player_state.duration = track->trackInfo.duration;
      //    this->ctx->timeProvider->getSyncedTimestamp();
      // putPlayerState(PutStateReason::PutStateReason_PICKER_OPENED);
      sendCommand(CommandType::PLAYBACK, trackQueue->preloadedTracks[0]);
    } else
      putPlayerState();
  };

  this->trackPlayer = std::make_shared<TrackPlayer>(
      ctx, trackQueue, onTrackEnd, onTrackChanged,
      &device.player_state.options.repeating_track);
  CSPOT_LOG(info, "Started player");

  auto connectStateSubscription = [this](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return;
    if (strstr(res.mercuryHeader.uri, "v1/devices/")) {
      putDeviceState(PutStateReason::PutStateReason_SPIRC_NOTIFY);
    } else if (strstr(res.mercuryHeader.uri, "player/command")) {
      if (res.parts[0].size())
        parseCommand(res.parts[0]);
    } else if (strstr(res.mercuryHeader.uri, "volume")) {
      if (res.parts[0].size()) {
        SetVolumeCommand newVolume;
        pbDecode(newVolume, SetVolumeCommand_fields, res.parts[0]);
        device.device_info.volume = newVolume.volume;
        device.device_info.has_volume = true;
        sendCommand(CommandType::VOLUME, newVolume.volume);
        pb_release(SetVolumeCommand_fields, &newVolume);
      }
    } else if (strstr(res.mercuryHeader.uri, "cluster")) {
      if (0) {  // will send cluster info if new device logged in, but connect_state/cluster is called way too often(for example during each song) too send each time a putPlayerStateRequest
                //if(is_active){
        putPlayerState();
      }
    } else
      CSPOT_LOG(debug, "Unknown connect_state, uri : %s",
                res.mercuryHeader.uri);
  };

  this->ctx->session->addSubscriptionListener("hm://connect-state/",
                                              connectStateSubscription);
  CSPOT_LOG(info, "Added connect-state subscription");

  // the device connection status gets reported trough "hm://social-connect",if active
  auto socialConnectSubscription = [this](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return;
    if (res.parts[0].size()) {
      auto jsonResult = nlohmann::json::parse(res.parts[0]);
      if (jsonResult.find("deviceBroadcastStatus") != jsonResult.end()) {
        if (jsonResult.find("deviceBroadcastStatus")->find("device_id") !=
            jsonResult.find("deviceBroadcastStatus")->end()) {
          if (jsonResult.find("deviceBroadcastStatus")
                  ->at("device_id")
                  .get<std::string>() != this->ctx->config.deviceId)
            goto changePlayerState;
        }
      } else if (jsonResult.find("reason") != jsonResult.end() &&
                 jsonResult.at("reason") == "SESSION_DELETED")
        goto changePlayerState;
      return;
    changePlayerState:
      if (this->is_active) {
        this->ctx->playbackMetrics->end_reason = PlaybackMetrics::REMOTE;
        this->ctx->playbackMetrics->end_source = "unknown";
        this->trackPlayer->stop();
        this->is_active = false;
        if (device.player_state.has_restrictions)
          pb_release(Restrictions_fields, &device.player_state.restrictions);
        device.player_state.restrictions = Restrictions_init_zero;
        device.player_state.has_restrictions = false;
        this->putDeviceState(PutStateReason::PutStateReason_BECAME_INACTIVE);
        CSPOT_LOG(debug, "Device changed");
        sendCommand(CommandType::DISC);
#ifndef CONFIG_CSPOT_STAY_CONNECTED_ON_TRANSFER
        this->disconnect();
#endif
      }
    }
  };

  this->ctx->session->addSubscriptionListener("social-connect",
                                              socialConnectSubscription);
  CSPOT_LOG(info, "Added social-connect supscription");

  ctx->session->setConnectedHandler([this]() {
    CSPOT_LOG(info, "Registered new device");
    this->putDeviceState(
        PutStateReason_SPIRC_HELLO);  // : PutStateReason::PutStateReason_NEW_DEVICE);
    // Assign country code
    this->ctx->config.countryCode = this->ctx->session->getCountryCode();
  });

  device = {};

  // Prepare default device state
  device.has_device_info = true;

  // Prepare device info
  device.device_info.can_play = true;
  device.device_info.has_can_play = true;

  device.device_info.has_volume = true;
  device.device_info.volume = ctx->config.volume;

  device.device_info.name = strdup(ctx->config.deviceName.c_str());

  device.device_info.has_capabilities = true;
  device.device_info.capabilities = Capabilities{
      true,
      1,  //can_be_player
      true,
      1,  //restrict_to_local
      true,
      1,  //gaia_eq_connect_id
      true,
      0,  //supports_logout
      true,
      1,  //is_observable
      true,
      64,  //volume_steps
      0,
      NULL,  //{"audio/track", "audio/episode", "audio/episode+track"}, //supported_types
      true,
      1,  //command_acks
      false,
      0,  //supports_rename
      false,
      0,  //hidden
      false,
      0,  //disable_volume
      false,
      0,  //connect_disabled
      true,
      1,  //supports_playlist_v2
      true,
      1,  //is_controllable
      true,
      1,  //supports_external_episodes
      true,
      0,  //supports_set_backend_metadata
      true,
      1,  //supports_transfer_command
      false,
      0,  //supports_command_request
      false,
      0,  //is_voice_enabled
      true,
      0,  //needs_full_player_state //overuses MercuryManager, but keeps connection for outside wlan alive
      false,
      0,  //supports_gzip_pushes
      false,
      0,  //supports_lossless_audio
      true,
      1,  //supports_set_options_command
      true,  {false, 0, false, 0, true, 1}};
  device.device_info.capabilities.supported_types =
      (char**)calloc(5, sizeof(char*));
  device.device_info.capabilities.supported_types[0] = strdup("audio/track");
  device.device_info.capabilities.supported_types[1] = strdup("audio/episode");
  device.device_info.capabilities.supported_types[2] =
      strdup("audio/episode+track");
  device.device_info.capabilities.supported_types[3] =
      strdup("audio/interruption");
  device.device_info.capabilities.supported_types[4] = strdup("audio/local");
  device.device_info.capabilities.supported_types_count = 5;
  device.device_info.device_software_version = strdup(swVersion);
  device.device_info.has_device_type = true;
  device.device_info.device_type = DeviceType::DeviceType_SPEAKER;
  device.device_info.spirc_version = strdup(protocolVersion);
  device.device_info.device_id = strdup(ctx->config.deviceId.c_str());
  //device.device_info.client_id
  device.device_info.brand = strdup(brandName);
  device.device_info.model = strdup(informationString);
  //device.device_info.metadata_map = {{"debug_level","1"},{"tier1_port","0"},{"device_address_mask",local_ip}};
  //device.device_info.public_ip = ; // gets added trough server
  //device.device_info.license = ;
}

DeviceStateHandler::~DeviceStateHandler() {
  TrackReference::clearProvidedTracklist(&currentTracks);
  device.player_state.track = ProvidedTrack_init_zero;
  pb_release(Device_fields, &device);
}

void DeviceStateHandler::putDeviceState(PutStateReason put_state_reason) {
  //std::scoped_lock lock(playerStateMutex);
  std::string uri =
      "hm://connect-state/v1/devices/" + this->ctx->config.deviceId + "/";

  std::vector<ProvidedTrack> send_tracks = {};
  PutStateRequest tempPutReq = PutStateRequest_init_zero;
  tempPutReq.has_device = true;
  tempPutReq.has_member_type = true;
  tempPutReq.member_type = MemberType::MemberType_CONNECT_STATE;
  tempPutReq.has_is_active = true;
  tempPutReq.is_active = is_active;
  tempPutReq.has_put_state_reason = true;
  tempPutReq.put_state_reason = put_state_reason;
  tempPutReq.has_message_id = true;
  tempPutReq.message_id = last_message_id;
  tempPutReq.has_has_been_playing_for_ms = true;
  tempPutReq.has_been_playing_for_ms = (uint64_t)-1;
  tempPutReq.has_client_side_timestamp = true;
  tempPutReq.client_side_timestamp =
      this->ctx->timeProvider->getSyncedTimestamp();
  tempPutReq.has_only_write_player_state = true;
  tempPutReq.only_write_player_state = false;

  if (is_active) {
    tempPutReq.has_started_playing_at = true;
    tempPutReq.started_playing_at = this->started_playing_at;
    tempPutReq.has_been_playing_for_ms =
        this->ctx->timeProvider->getSyncedTimestamp() -
        this->started_playing_at;
    device.has_player_state = true;
    device.player_state.has_position_as_of_timestamp = true;
    device.player_state.position_as_of_timestamp =
        this->ctx->timeProvider->getSyncedTimestamp() -
        device.player_state.timestamp;
  } else
    device.has_player_state = false;
  device.player_state.next_tracks.funcs.encode =
      &cspot::TrackReference::pbEncodeProvidedTracks;
  device.player_state.next_tracks.arg = &queuePacket;
  tempPutReq.device = this->device;

  auto putStateRequest = pbEncode(PutStateRequest_fields, &tempPutReq);
  tempPutReq.device = Device_init_zero;
  pb_release(PutStateRequest_fields, &tempPutReq);
  auto parts = MercurySession::DataParts({putStateRequest});
  auto responseLambda = [this](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return;
  };
  this->ctx->session->execute(MercurySession::RequestType::PUT, uri,
                              responseLambda, parts);
}

void DeviceStateHandler::putPlayerState(PutStateReason put_state_reason) {
  //std::scoped_lock lock(playerStateMutex);
  std::string uri =
      "hm://connect-state/v1/devices/" + this->ctx->config.deviceId + "/";
  PutStateRequest tempPutReq = {};
  pb_release(PutStateRequest_fields, &tempPutReq);
  tempPutReq = PutStateRequest_init_zero;
  tempPutReq.has_device = true;
  tempPutReq.has_member_type = false;
  tempPutReq.member_type = MemberType::MemberType_CONNECT_STATE;
  tempPutReq.has_is_active = true;
  tempPutReq.is_active = true;
  tempPutReq.has_put_state_reason = true;
  tempPutReq.put_state_reason = put_state_reason;
  tempPutReq.last_command_message_id = last_message_id;
  tempPutReq.has_started_playing_at = true;
  tempPutReq.started_playing_at =
      this->trackQueue->preloadedTracks[0]->trackMetrics->trackHeaderTime;
  tempPutReq.has_has_been_playing_for_ms = true;
  tempPutReq.has_been_playing_for_ms =
      this->ctx->timeProvider->getSyncedTimestamp() -
      this->trackQueue->preloadedTracks[0]->trackMetrics->trackHeaderTime;
  tempPutReq.has_client_side_timestamp = true;
  tempPutReq.client_side_timestamp =
      this->ctx->timeProvider->getSyncedTimestamp();
  tempPutReq.has_only_write_player_state = true;
  tempPutReq.only_write_player_state = true;
  device.player_state.has_position_as_of_timestamp = true;
  device.player_state.timestamp =
      trackQueue->preloadedTracks[0]->trackMetrics->currentInterval->start;
  device.player_state.position_as_of_timestamp =
      (int64_t)trackQueue->preloadedTracks[0]->trackMetrics->getPosition();
  device.has_player_state = true;
  device.player_state.has_position_as_of_timestamp = true;
  device.player_state.position_as_of_timestamp =
      trackQueue->preloadedTracks[0]->trackMetrics->getPosition();
  queuePacket = {&offset, &currentTracks};
  device.player_state.next_tracks.funcs.encode =
      &cspot::TrackReference::pbEncodeProvidedTracks;
  device.player_state.next_tracks.arg = &queuePacket;
  if (device.player_state.track.provider &&
      strcmp(device.player_state.track.provider, "autoplay") == 0) {
    if (device.player_state.has_restrictions)
      pb_release(Restrictions_fields, &device.player_state.restrictions);
    pb_release(ContextIndex_fields, &device.player_state.index);
    device.player_state.index = ContextIndex_init_zero;
    device.player_state.has_index = false;
    device.player_state.restrictions = Restrictions_init_zero;
    if (!device.player_state.is_paused) {
      device.player_state.restrictions.disallow_resuming_reasons =
          (char**)calloc(1, sizeof(char*));
      device.player_state.restrictions.disallow_resuming_reasons_count = 1;
      device.player_state.restrictions.disallow_resuming_reasons[0] =
          strdup("not_paused");
    } else {
      device.player_state.restrictions.disallow_pausing_reasons =
          (char**)calloc(1, sizeof(char*));
      device.player_state.restrictions.disallow_pausing_reasons_count = 1;
      device.player_state.restrictions.disallow_pausing_reasons[0] =
          strdup("not_playing");
    }
    // Update player state with current track information
    device.player_state.restrictions.disallow_toggling_repeat_context_reasons =
        (char**)calloc(3, sizeof(char*));
    device.player_state.restrictions
        .disallow_toggling_repeat_context_reasons_count = 3;
    device.player_state.restrictions
        .disallow_toggling_repeat_context_reasons[0] = strdup("autoplay");
    device.player_state.restrictions
        .disallow_toggling_repeat_context_reasons[1] =
        strdup("endless_context");
    device.player_state.restrictions
        .disallow_toggling_repeat_context_reasons[2] = strdup("radio");

    device.player_state.restrictions.disallow_toggling_repeat_track_reasons =
        (char**)calloc(1, sizeof(char*));
    device.player_state.restrictions
        .disallow_toggling_repeat_track_reasons_count = 1;
    device.player_state.restrictions.disallow_toggling_repeat_track_reasons[0] =
        strdup("autoplay");

    device.player_state.restrictions.disallow_toggling_shuffle_reasons =
        (char**)calloc(3, sizeof(char*));
    device.player_state.restrictions.disallow_toggling_shuffle_reasons_count =
        3;
    device.player_state.restrictions.disallow_toggling_shuffle_reasons[0] =
        strdup("autoplay");
    device.player_state.restrictions.disallow_toggling_shuffle_reasons[1] =
        strdup("endless_context");
    device.player_state.restrictions.disallow_toggling_shuffle_reasons[2] =
        strdup("radio");

    device.player_state.restrictions.disallow_loading_context_reasons =
        (char**)calloc(1, sizeof(char*));
    device.player_state.restrictions.disallow_loading_context_reasons_count = 1;
    device.player_state.restrictions.disallow_loading_context_reasons[0] =
        strdup("not_supported_by_content_type");

    device.player_state.has_index = false;
    device.player_state.has_restrictions = true;
    if (device.player_state.play_origin.feature_classes != NULL) {
      free(device.player_state.play_origin.feature_classes);
      device.player_state.play_origin.feature_classes = NULL;
    }
  } else {
    device.player_state.index =
        ContextIndex{true, device.player_state.track.page, true,
                     device.player_state.track.original_index};
    if (device.player_state.has_restrictions)
      pb_release(Restrictions_fields, &device.player_state.restrictions);
    device.player_state.restrictions = Restrictions_init_zero;
    if (!device.player_state.is_paused) {
      device.player_state.restrictions.disallow_resuming_reasons =
          (char**)calloc(1, sizeof(char*));
      device.player_state.restrictions.disallow_resuming_reasons_count = 1;
      device.player_state.restrictions.disallow_resuming_reasons[0] =
          strdup("not_paused");
    } else {
      device.player_state.restrictions.disallow_pausing_reasons =
          (char**)calloc(1, sizeof(char*));
      device.player_state.restrictions.disallow_pausing_reasons_count = 1;
      device.player_state.restrictions.disallow_pausing_reasons[0] =
          strdup("not_playing");
    }
    device.player_state.restrictions.disallow_loading_context_reasons =
        (char**)calloc(1, sizeof(char*));
    device.player_state.restrictions.disallow_loading_context_reasons_count = 1;
    device.player_state.restrictions.disallow_loading_context_reasons[0] =
        strdup("not_supported_by_content_type");

    device.player_state.has_restrictions = true;
  }
  tempPutReq.device = this->device;
  auto putStateRequest = pbEncode(PutStateRequest_fields, &tempPutReq);
  tempPutReq.device = Device_init_zero;
  pb_release(PutStateRequest_fields, &tempPutReq);
  auto parts = MercurySession::DataParts({putStateRequest});

  auto responseLambda = [this](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return;
  };
  this->ctx->session->execute(MercurySession::RequestType::PUT, uri,
                              responseLambda, parts);
}

void DeviceStateHandler::disconnect() {
  if (this->is_active) {
    this->ctx->playbackMetrics->end_reason = PlaybackMetrics::REMOTE;
    this->ctx->playbackMetrics->end_source = "unknown";
    this->trackPlayer->stop();
    this->is_active = false;
    if (device.player_state.has_restrictions)
      pb_release(Restrictions_fields, &device.player_state.restrictions);
    device.player_state.restrictions = Restrictions_init_zero;
    device.player_state.has_restrictions = false;
    this->putDeviceState(PutStateReason::PutStateReason_BECAME_INACTIVE);
    CSPOT_LOG(debug, "Device changed");
    sendCommand(CommandType::DISC);
  }
  this->trackQueue->preloadedTracks.clear();
  this->trackQueue->stopTask();
  this->ctx->session->disconnect();
  this->onClose();
}

void DeviceStateHandler::skip(CommandType dir, bool notify) {
  if (dir == CommandType::SKIP_NEXT) {
    this->device.player_state.track = currentTracks[offset];
    if (this->device.player_state.track.full_metadata_count >
        this->device.player_state.track.metadata_count)
      this->device.player_state.track.metadata_count =
          this->device.player_state.track.full_metadata_count;
    if (trackQueue->preloadedTracks.size()) {
      trackQueue->preloadedTracks.pop_front();
      if (currentTracks.size() >
          (trackQueue->preloadedTracks.size() + offset)) {
        while (currentTracks.size() >
                   trackQueue->preloadedTracks.size() + offset &&
               trackQueue->preloadedTracks.size() < 3) {
          trackQueue->preloadedTracks.push_back(
              std::make_shared<cspot::QueuedTrack>(
                  currentTracks[offset + trackQueue->preloadedTracks.size()],
                  this->ctx, this->trackQueue->playableSemaphore));
        }
      }
      offset++;
    }
  } else if (trackQueue->preloadedTracks[0]->trackMetrics->getPosition() >=
                 3000 &&
             offset > 1) {
    trackQueue->preloadedTracks.pop_back();
    offset--;
    trackQueue->preloadedTracks.push_front(std::make_shared<cspot::QueuedTrack>(
        currentTracks[offset - 1], this->ctx,
        this->trackQueue->playableSemaphore));
  } else {
    if (trackQueue->preloadedTracks.size())
      trackQueue->preloadedTracks[0]->requestedPosition = 0;
  }
  if (trackQueue->preloadedTracks.size() &&
      currentTracks.size() < offset + trackQueue->preloadedTracks.size()) {
    playerContext->resolveTracklist(metadata_map, reloadTrackList);
  }
  if (!trackQueue->preloadedTracks.size())
    this->trackPlayer->stop();
  else if (!notify)
    trackPlayer->resetState();
}

void DeviceStateHandler::parseCommand(std::vector<uint8_t>& data) {
  if (data.size() <= 2)
    return;
  nlohmann::json jsonResult;
  try {
    jsonResult = nlohmann::json::parse(data);
  } catch (const nlohmann::json::parse_error&) {
    CSPOT_LOG(error, "Failed to parse command");
    return;  // Parsing failed
  }
  last_message_id = jsonResult.value("message_id", last_message_id);

  auto command = jsonResult.find("command");
  if (command != jsonResult.end()) {
    if (command->find("endpoint") == command->end())
      return;
    CSPOT_LOG(debug, "Parsing new command, endpoint : %s",
              command->at("endpoint").get<std::string>().c_str());

    auto options = command->find("options");
    if (command->at("endpoint") == "transfer") {
      if (is_active)
        return;
      if (options != command->end()) {
        if (options->find("restore_paused") !=
            options->end()) {  //"restore"==play
          if (!is_active && options->at("restore_paused") == "restore") {
            started_playing_at = this->ctx->timeProvider->getSyncedTimestamp();
            is_active = true;
          }
        }
      }
      if (this->playerContext->next_page_url != NULL)
        unreference(&(this->playerContext->next_page_url));
      this->playerContext->radio_offset = 0;
      this->device.player_state.has_is_playing = true;
      this->device.player_state.is_playing = true;
      this->device.player_state.has_timestamp = true;
      this->device.player_state.timestamp =
          this->ctx->timeProvider->getSyncedTimestamp();
      if (!is_active) {
        started_playing_at = this->ctx->timeProvider->getSyncedTimestamp();
        is_active = true;
      }
      auto logging_params = command->find("logging_params");
      if (logging_params != command->end()) {
        metadata_map.clear();
        if (logging_params->find("page_instance_ids") !=
            logging_params->end()) {
          metadata_map.push_back(std::make_pair(
              "page_instance_id",
              logging_params->at("page_instance_ids")[0].get<std::string>()));
        }

        if (logging_params->find("interaction_ids") != logging_params->end()) {
          metadata_map.push_back(std::make_pair(
              "interaction_id",
              logging_params->at("interaction_ids")[0].get<std::string>()));
        }
      }
      auto responseHandler = [this](MercurySession::Response res) {
        if (res.fail || !res.parts.size())
          return;
        cspot::TrackReference::clearProvidedTracklist(&currentTracks);
        currentTracks = {};
        Cluster cluster = {};
        for (int i = this->device.player_state.context_metadata_count - 1;
             i >= 0; i--) {
          unreference(&(this->device.player_state.context_metadata[i].key));
          unreference(&(this->device.player_state.context_metadata[i].value));
        }
        free(this->device.player_state.context_metadata);
        this->device.player_state.context_metadata = NULL;
        this->device.player_state.context_metadata_count = 0;
        device.player_state.track = ProvidedTrack_init_zero;
        device.player_state.next_tracks.arg = NULL;
        if (device.player_state.has_restrictions)
          pb_release(Restrictions_fields, &device.player_state.restrictions);
        device.player_state.restrictions = Restrictions_init_zero;
        device.player_state.has_restrictions = false;
        pb_release(PlayerState_fields, &this->device.player_state);
        this->device.player_state = PlayerState_init_zero;
        pb_release(Cluster_fields, &cluster);
        cluster.player_state.next_tracks.funcs.decode =
            &cspot::TrackReference::pbDecodeProvidedTracks;
        cluster.player_state.next_tracks.arg = &this->currentTracks;

        pbDecode(cluster, Cluster_fields, res.parts[0]);
        this->device.player_state = cluster.player_state;
        cluster.player_state = PlayerState_init_zero;
        pb_release(Cluster_fields, &cluster);
        offsetFromStartInMillis = device.player_state.position_as_of_timestamp;

        std::vector<uint8_t> random_bytes;
        static std::uniform_int_distribution<int> d(0, 255);
        for (int i = 0; i < 16; i++) {
          random_bytes.push_back(d(ctx->rng));
        }
        unreference(&(this->device.player_state.session_id));
        this->device.player_state.session_id =
            strdup(bytesToHexString(random_bytes).c_str());

        unreference(&(this->device.player_state.playback_id));
        random_bytes.clear();
        for (int i = 0; i < 16; i++) {
          random_bytes.push_back(d(ctx->rng));
        }
        this->device.player_state.playback_id =
            strdup(base64Encode(random_bytes).c_str());

        this->currentTracks.insert(this->currentTracks.begin(),
                                   this->device.player_state.track);
        offset = 0;

        queuePacket = {&offset, &currentTracks};
        this->putDeviceState(
            PutStateReason::PutStateReason_PLAYER_STATE_CHANGED);

        trackQueue->preloadedTracks.clear();
        reloadPreloadedTracks = true;
        playerContext->resolveTracklist(metadata_map, reloadTrackList, true);
      };
      this->ctx->session->execute(MercurySession::RequestType::GET,
                                  "hm://connect-state/v1/cluster",
                                  responseHandler);
    } else if (this->is_active) {
      if (command->at("endpoint") == "play") {
#ifndef CONFIG_BELL_NOCODEC
        handler->trackPlayer->stop();
        sendCommand(CommandType::DEPLETED);
#endif
        if (this->playerContext->next_page_url != NULL)
          unreference(&(this->playerContext->next_page_url));
        this->playerContext->radio_offset = 0;
        trackQueue->preloadedTracks.clear();
        uint8_t queued = 0;
        ProvidedTrack track = ProvidedTrack_init_zero;
        if (!this->device.player_state.is_playing) {
          this->device.player_state.is_playing = true;
          this->device.player_state.has_track = true;
        }
        for (int i = 0; i < currentTracks.size(); i++) {
          if (i > this->offset || currentTracks[i].provider == NULL ||
              strcmp(currentTracks[i].provider, "queue") != 0) {
            cspot::TrackReference::pbReleaseProvidedTrack(&currentTracks[i]);
            currentTracks.erase(currentTracks.begin() + i);
            i--;
          }
        }

        auto logging_params = command->find("logging_params");
        if (logging_params != command->end()) {
          metadata_map.clear();
          if (logging_params->find("page_instance_ids") !=
              logging_params->end()) {
            metadata_map.push_back(std::make_pair(
                "page_instance_ids",
                logging_params->at("page_instance_ids")[0].get<std::string>()));
          }
          if (logging_params->find("interaction_ids") !=
              logging_params->end()) {
            metadata_map.push_back(std::make_pair(
                "interaction_id",
                logging_params->at("interaction_ids")[0].get<std::string>()));
          }
        }

        if (command->find("play_origin") != command->end()) {
          pb_release(PlayOrigin_fields, &device.player_state.play_origin);
          device.player_state.play_origin = PlayOrigin_init_zero;
          device.player_state.play_origin.feature_identifier =
              PlayerContext::createStringReferenceIfFound(
                  command->at("play_origin"), "feature_identifier");
          device.player_state.play_origin.feature_version =
              PlayerContext::createStringReferenceIfFound(
                  command->at("play_origin"), "feature_version");
          device.player_state.play_origin.referrer_identifier =
              PlayerContext::createStringReferenceIfFound(
                  command->at("play_origin"), "referrer_identifier");
        }

        auto options = command->find("options");
        int64_t playlist_offset = 0;
        if (options != command->end()) {
          if (options->find("player_options_override") != options->end() &&
              options->at("player_options_override")
                      .find("shuffling_context") !=
                  options->at("player_options_override").end())
            device.player_state.options.shuffling_context =
                options->at("player_options_override").at("shuffling_context");
          else
            device.player_state.options.shuffling_context = false;
          if (options->find("skip_to") != options->end()) {
            if (options->at("skip_to").size()) {
              if (options->at("skip_to").find("track_index") !=
                  options->at("skip_to").end()) {
                playlist_offset = options->at("skip_to").at("track_index");
                track.original_index = playlist_offset;
              }
              track.uri = PlayerContext::createStringReferenceIfFound(
                  options->at("skip_to"), "track_uri");
              track.uid = PlayerContext::createStringReferenceIfFound(
                  options->at("skip_to"), "track_uid");
            }
          }
        }

        auto metadata = command->at("context").find("metadata");
        if (metadata != command->at("context").end()) {
          if (metadata->find("enhanced_context") != metadata->end()) {
            this->device.player_state.options.context_enhancement[0].key =
                strdup("context_enhancement");
            this->device.player_state.options.context_enhancement[0].value =
                strdup("NONE");
            this->device.player_state.options.context_enhancement_count = 1;
          } else if (this->device.player_state.options
                         .context_enhancement_count) {
            for (auto& enhamcement :
                 this->device.player_state.options.context_enhancement) {
              this->unreference(&(enhamcement.key));
              this->unreference(&(enhamcement.value));
            }
            this->device.player_state.options.context_enhancement_count = 0;
          }

          context_metadata_map.clear();
          for (auto element : metadata->items()) {
            if (element.value().size() && element.value() != "" &&
                element.key() != "canContainArtists.uris") {
              context_metadata_map.push_back(std::make_pair(
                  element.key(), element.value().get<std::string>()));
            }
          }
          for (int i = this->device.player_state.context_metadata_count - 1;
               i >= 0; i--) {
            unreference(&(this->device.player_state.context_metadata[i].key));
            unreference(&(this->device.player_state.context_metadata[i].value));
          }
          free(this->device.player_state.context_metadata);
          this->device.player_state.context_metadata =
              (PlayerState_ContextMetadataEntry*)calloc(
                  context_metadata_map.size(),
                  sizeof(PlayerState_ContextMetadataEntry));
          for (int i = 0; i < context_metadata_map.size(); i++) {
            this->device.player_state.context_metadata[i].key =
                strdup(context_metadata_map[i].first.c_str());
            this->device.player_state.context_metadata[i].value =
                strdup(context_metadata_map[i].second.c_str());
          }
          this->device.player_state.context_metadata_count =
              context_metadata_map.size();
        } else {
          if (this->device.player_state.options.context_enhancement_count) {
            for (auto& enhamcement :
                 this->device.player_state.options.context_enhancement) {
              this->unreference(&(enhamcement.key));
              this->unreference(&(enhamcement.value));
            }
            this->device.player_state.options.context_enhancement_count = 0;
          }
          context_metadata_map.clear();
          for (int i = this->device.player_state.context_metadata_count - 1;
               i >= 0; i--) {
            unreference(&(this->device.player_state.context_metadata[i].key));
            unreference(&(this->device.player_state.context_metadata[i].value));
          }
          free(this->device.player_state.context_metadata);
          this->device.player_state.context_metadata = NULL;
          this->device.player_state.context_metadata_count = 0;
        }

        unreference(&(this->device.player_state.context_uri));
        this->device.player_state.context_uri =
            PlayerContext::createStringReferenceIfFound(command->at("context"),
                                                        "uri");
        unreference(&(this->device.player_state.context_url));
        this->device.player_state.context_url =
            PlayerContext::createStringReferenceIfFound(command->at("context"),
                                                        "url");
        reloadPreloadedTracks = true;
        //this->trackPlayer->start();
        uint8_t metadata_offset = 0;
        for (auto metadata_entry : metadata_map) {
          track.metadata[metadata_offset].key =
              strdup(metadata_entry.first.c_str());
          track.metadata[metadata_offset].value =
              strdup(metadata_entry.second.c_str());
          metadata_offset++;
        }

        if (command->at("context").find("pages") !=
                command->at("context").end() &&
            command->at("context").at("pages")[0]["tracks"].size() >
                playlist_offset) {
          //populate first tarck
          if (track.uri == NULL) {
            track.uri = PlayerContext::createStringReferenceIfFound(
                command->at("context")["pages"][0]["tracks"][playlist_offset],
                "uri");
            track.uid = PlayerContext::createStringReferenceIfFound(
                command->at("context")["pages"][0]["tracks"][playlist_offset],
                "uid");
          }
          if (command->at("context")["pages"][0]["tracks"][playlist_offset]
                  .find("metadata") !=
              command->at("context")["pages"][0]["tracks"][playlist_offset]
                  .end()) {
            for (auto metadata_entry :
                 command->at("context")["pages"][0]["tracks"][playlist_offset]
                     .at("metadata")
                     .items()) {
              track.metadata[metadata_offset].key =
                  strdup(metadata_entry.key().c_str());
              track.metadata[metadata_offset].value =
                  strdup(((std::string)metadata_entry.value()).c_str());
              metadata_offset++;
            }
          }
        }
        track.full_metadata_count = metadata_offset;
        track.metadata_count = metadata_offset;
        if (this->device.player_state.context_url != NULL &&
            strchr(this->device.player_state.context_url, ':') != NULL) {
          track.provider =
              strndup(this->device.player_state.context_url,
                      strchr(this->device.player_state.context_url, ':') -
                          this->device.player_state.context_url);
        } else if (strchr(this->device.player_state.context_uri, ':') !=
                   strrchr(this->device.player_state.context_uri, ':')) {
          track.provider = strdup("context");
        }
        if (track.uri) {
          currentTracks.insert(currentTracks.begin(), track);
          device.player_state.track = track;
        } else
          pb_release(ProvidedTrack_fields, &track);
        offset = 0;
        this->playerContext->resolveTracklist(metadata_map, reloadTrackList,
                                              true, true);
        CSPOT_LOG(info, "Tracklist reloaded");
      } else if (command->at("endpoint") == "pause") {
        device.player_state.is_paused = true;
        device.player_state.has_is_paused = true;
        this->putPlayerState();
        sendCommand(CommandType::PAUSE);
      } else if (command->at("endpoint") == "resume") {
        device.player_state.is_paused = false;
        device.player_state.has_is_paused = true;
        this->putPlayerState();
        sendCommand(CommandType::PLAY);
      } else if (command->at("endpoint") == "skip_next") {
        ctx->playbackMetrics->end_reason = PlaybackMetrics::FORWARD_BTN;
#ifndef CONFIG_BELL_NOCODEC
        this->needsToBeSkipped = false;
#endif
        if (command->find("track") == command->end())
          skip(CommandType::SKIP_NEXT, false);
        else {
          offset = 0;
          for (auto track : currentTracks) {
            if (strcmp(command->find("track")
                           ->at("uri")
                           .get<std::string>()
                           .c_str(),
                       track.uri) == 0)
              break;
            offset++;
          }
          trackQueue->preloadedTracks.clear();

          this->device.player_state.track = currentTracks[offset];
          for (auto i = offset;
               i < (currentTracks.size() < 3 + offset ? currentTracks.size()
                                                      : 3 + offset);
               i++) {
            trackQueue->preloadedTracks.push_back(
                std::make_shared<cspot::QueuedTrack>(
                    currentTracks[i], this->ctx,
                    this->trackQueue->playableSemaphore));
          }
          offset++;
          trackPlayer->resetState();
        }
        sendCommand(CommandType::SKIP_NEXT);
      } else if (command->at("endpoint") == "skip_prev") {
        ctx->playbackMetrics->end_reason = PlaybackMetrics::BACKWARD_BTN;
        needsToBeSkipped = false;
        skip(CommandType::SKIP_PREV, false);
        sendCommand(CommandType::SKIP_PREV);

      } else if (command->at("endpoint") == "seek_to") {

#ifndef CONFIG_BELL_NOCODEC
        if (!this->trackQueue->preloadedTracks[0]->loading)
          needsToBeSkipped = false;
#endif
        if (command->at("relative") == "beginning") {  //relative
          this->device.player_state.has_position_as_of_timestamp = true;
          this->device.player_state.position_as_of_timestamp =
              command->at("value").get<int64_t>();
          this->device.player_state.timestamp =
              this->ctx->timeProvider->getSyncedTimestamp();
          this->trackPlayer->seekMs(
              command->at("value").get<uint32_t>(),
              this->trackQueue->preloadedTracks[0]->loading);
        } else if (command->at("relative") == "current") {
          this->device.player_state.has_position_as_of_timestamp = true;
          this->device.player_state.position_as_of_timestamp =
              command->at("value").get<int64_t>() +
              command->at("position").get<int64_t>();
          this->trackPlayer->seekMs(
              this->device.player_state.position_as_of_timestamp,
              this->trackQueue->preloadedTracks[0]->loading);
          this->device.player_state.timestamp =
              this->ctx->timeProvider->getSyncedTimestamp();
        }
        sendCommand(
            CommandType::SEEK,
            (int32_t)this->device.player_state.position_as_of_timestamp);
        this->putPlayerState();
      } else if (command->at("endpoint") == "add_to_queue") {
        uint8_t queuedOffset = 0;
        //look up already queued tracks
        for (uint8_t i = offset; i < currentTracks.size(); i++) {
          if (strcmp(currentTracks[i].provider, "queue") != 0)
            break;
          queuedOffset++;
        }

        ProvidedTrack track = {};
        track.uri = strdup(
            command->find("track")->at("uri").get<std::string>().c_str());
        track.provider = strdup("queue");
        this->currentTracks.insert(
            this->currentTracks.begin() + offset + queuedOffset, track);
        if (queuedOffset < 2) {
          trackQueue->preloadedTracks.pop_back();
          trackQueue->preloadedTracks.insert(
              trackQueue->preloadedTracks.begin() + 1 + queuedOffset,
              std::make_shared<cspot::QueuedTrack>(
                  currentTracks[offset + queuedOffset], this->ctx,
                  this->trackQueue->playableSemaphore));
        }
#ifndef CONFIG_BELL_NOCODEC
        this->trackPlayer->seekMs(
            trackQueue->preloadedTracks[0]->trackMetrics->getPosition(),
            this->trackQueue->preloadedTracks[0]->loading);
        sendCommand(
            CommandType::SEEK,
            (int32_t)this->device.player_state.position_as_of_timestamp);
#endif
        this->putPlayerState();
      } else if (command->at("endpoint") == "set_queue") {
        uint8_t queuedOffset = 0, newQueuedOffset = 0;
        //look up already queued tracks
        for (uint8_t i = offset; i < currentTracks.size(); i++) {
          if (strcmp(currentTracks[i].provider, "queue") != 0)
            break;
          queuedOffset++;
        }
        auto tracks = command->find("next_tracks");
        if (!command->at("next_tracks").size()) {
          for (uint8_t i = offset; i < currentTracks.size(); i++) {
            if (strcmp(currentTracks[i].provider, "queue") != 0)
              break;
            cspot::TrackReference::pbReleaseProvidedTrack(&currentTracks[i]);
            currentTracks.erase(currentTracks.begin() + i);
            i--;
          }
        }
        if (command->find("next_tracks") != command->end()) {
          for (int i = 0; i < command->at("next_tracks").size(); i++) {
            if (strcmp(command->at("next_tracks")[i]["uri"]
                           .get<std::string>()
                           .c_str(),
                       currentTracks[offset + i].uri) != 0) {
              if (newQueuedOffset < queuedOffset) {
                queuedOffset--;
                goto removeTrack;
              } else if (command->at("next_tracks")[i]["provider"] == "queue") {
                ProvidedTrack track = {};
                track.uri = strdup(command->at("next_tracks")[i]["uri"]
                                       .get<std::string>()
                                       .c_str());
                track.provider = strdup("queue");
                this->currentTracks.insert(
                    this->currentTracks.begin() + offset + i, track);
                continue;
              }
            removeTrack:;
              cspot::TrackReference::pbReleaseProvidedTrack(
                  &currentTracks[offset + i]);
              currentTracks.erase(currentTracks.begin() + offset + i);
              if (strcmp(currentTracks[offset + i].provider, "queue") != 0 ||
                  strcmp(command->at("next_tracks")[i]["uri"]
                             .get<std::string>()
                             .c_str(),
                         currentTracks[offset + i].uri) == 0)
                break;
              i--;
            } else if (command->at("next_tracks")[i]["provider"] == "queue")
              newQueuedOffset++;
          }
        }
        if (queuedOffset < 2 || newQueuedOffset < 2) {
          trackQueue->preloadedTracks.clear();
          while (trackQueue->preloadedTracks.size() < 3)
            trackQueue->preloadedTracks.push_back(
                std::make_shared<cspot::QueuedTrack>(
                    currentTracks[offset + trackQueue->preloadedTracks.size() -
                                  1],
                    this->ctx, this->trackQueue->playableSemaphore));
        }
#ifndef CONFIG_BELL_NOCODEC
        this->trackPlayer->seekMs(
            trackQueue->preloadedTracks[0]->trackMetrics->getPosition(),
            this->trackQueue->preloadedTracks[0]->loading);
        sendCommand(
            CommandType::SEEK,
            (int32_t)this->device.player_state.position_as_of_timestamp);
#endif
        this->putPlayerState();
      } else if (command->at("endpoint") == "update_context") {
        unreference(&(this->device.player_state.session_id));
        this->device.player_state.session_id =
            PlayerContext::createStringReferenceIfFound(*command, "session_id");

        auto context = command->find("context");
        if (context != command->end()) {
          if (context_metadata_map.size())
            context_metadata_map.clear();
          context_uri = context->find("uri") != context->end()
                            ? context->at("uri").get<std::string>()
                            : " ";
          context_url = context->find("url") != context->end()
                            ? context->at("url").get<std::string>()
                            : " ";
          auto metadata = context->find("metadata");
          if (metadata != context->end()) {
            for (auto element : metadata->items()) {
              if (element.value().size() && element.value() != "") {
                context_metadata_map.push_back(std::make_pair(
                    element.key(), element.value().get<std::string>()));
              }
            }
          }
        }
      } else if (command->at("endpoint") == "set_shuffling_context") {
        if (context_uri.size()) {
          unreference(&(this->device.player_state.context_uri));
          this->device.player_state.context_uri = strdup(context_uri.c_str());
        }
        if (context_url.size()) {
          unreference(&(this->device.player_state.context_url));
          this->device.player_state.context_url = strdup(context_url.c_str());
        }
        for (int i = this->device.player_state.context_metadata_count - 1;
             i >= 0; i--) {
          unreference(&(this->device.player_state.context_metadata[i].key));
          unreference(&(this->device.player_state.context_metadata[i].value));
        }
        free(this->device.player_state.context_metadata);
        this->device.player_state.context_metadata =
            (PlayerState_ContextMetadataEntry*)calloc(
                context_metadata_map.size(),
                sizeof(PlayerState_ContextMetadataEntry));
        for (int i = 0; i < context_metadata_map.size(); i++) {
          this->device.player_state.context_metadata[i].key =
              strdup(context_metadata_map[i].first.c_str());
          this->device.player_state.context_metadata[i].value =
              strdup(context_metadata_map[i].second.c_str());
        }
        this->device.player_state.context_metadata_count =
            context_metadata_map.size();

        this->device.player_state.has_options = true;
        this->device.player_state.options.has_shuffling_context = true;
        if (command->find("value").value())
          this->device.player_state.options.shuffling_context = true;
        else
          this->device.player_state.options.shuffling_context = false;
        if (strchr(this->device.player_state.context_url, '?') != NULL) {
          this->device.player_state.options.context_enhancement[0].key =
              strdup("context_enhancement");
          this->device.player_state.options.context_enhancement[0].value =
              strdup("NONE");
          this->device.player_state.options.context_enhancement_count = 1;
        } else {
          if (this->device.player_state.options.context_enhancement_count) {
            unreference(&(
                this->device.player_state.options.context_enhancement[0].key));
            unreference(
                &(this->device.player_state.options.context_enhancement[0]
                      .value));
            this->device.player_state.options.context_enhancement_count = 0;
          }
        }
        playerStateChanged = true;
        this->trackQueue->preloadedTracks.erase(
            this->trackQueue->preloadedTracks.begin(),
            this->trackQueue->preloadedTracks.end());
        for (int i = offset; i < currentTracks.size(); i++) {
          if (strcmp(currentTracks[i].provider, "queue") != 0) {
            cspot::TrackReference::pbReleaseProvidedTrack(
                &currentTracks[offset + i]);
            currentTracks.erase(currentTracks.begin() + offset + i);
            i--;
          }
        }
        playerContext->resolveTracklist(metadata_map, reloadTrackList, true);
        sendCommand(CommandType::SET_SHUFFLE,
                    (int32_t)(this->device.player_state.options
                                      .context_enhancement_count
                                  ? 2
                                  : this->device.player_state.options
                                        .shuffling_context));
#ifndef CONFIG_BELL_NOCODEC
        this->trackPlayer->seekMs(
            trackQueue->preloadedTracks[0]->trackMetrics->getPosition(),
            this->trackQueue->preloadedTracks[0]->loading);
        sendCommand(
            CommandType::SEEK,
            (int32_t)this->device.player_state.position_as_of_timestamp);
#endif
      } else if (command->at("endpoint") == "set_options") {

        if (this->device.player_state.options.repeating_context !=
            command->at("repeating_context").get<bool>()) {
          uint8_t release = 0;
          for (int i = offset; i < currentTracks.size(); i++)
            if (strcmp(currentTracks[i].uri, "spotify:delimiter") == 0 ||
                strcmp(currentTracks[i].provider, "autoplay") == 0) {
              release = i;
              break;
            }
          if (release) {
            for (int i = release; i < currentTracks.size(); i++)
              cspot::TrackReference::pbReleaseProvidedTrack(&currentTracks[i]);
            currentTracks.erase(currentTracks.begin() + release,
                                currentTracks.end());
          }

          this->device.player_state.options.has_repeating_context = true;
          this->device.player_state.options.repeating_context =
              command->at("repeating_context").get<bool>();
          this->device.player_state.options.repeating_track =
              command->at("repeating_track").get<bool>();
          this->device.player_state.options.has_repeating_track = true;
          playerStateChanged = true;
          this->playerContext->resolveTracklist(metadata_map, reloadTrackList,
                                                true);
        } else {
          this->device.player_state.options.has_repeating_context = true;
          this->device.player_state.options.repeating_context =
              command->at("repeating_context").get<bool>();
          this->device.player_state.options.repeating_track =
              command->at("repeating_track").get<bool>();
          this->device.player_state.options.has_repeating_track = true;
          this->putPlayerState();
        }
        if (this->device.player_state.options.repeating_context)
          sendCommand(
              CommandType::SET_REPEAT,
              (int32_t)(this->device.player_state.options.repeating_context
                            ? 2
                            : this->device.player_state.options
                                  .repeating_track));
      } else
        CSPOT_LOG(error, "Unknown command: %s",
                  &command->at("endpoint").get<std::string>()[0]);
      return;
    }
  }
}
void DeviceStateHandler::sendCommand(CommandType type, CommandData data) {
  Command command;
  command.commandType = type;
  command.data = data;
  stateCallback(command);
}