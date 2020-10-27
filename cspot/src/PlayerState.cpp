#include "PlayerState.h"

PlayerState::PlayerState()
{
    // Prepare default state
    innerFrame = {};
    innerFrame.state.has_repeat = true;
    innerFrame.state.repeat = false;
    innerFrame.state.has_shuffle = true;
    innerFrame.state.shuffle = false;
    innerFrame.state.has_status = true;
    innerFrame.state.status = PlayStatus_kPlayStatusStop;
    innerFrame.state.has_position_measured_at = true;
    innerFrame.state.position_measured_at = 0;
    innerFrame.state.has_position_ms = true;
    innerFrame.state.position_ms = 0;

    innerFrame.device_state.sw_version = (char *)swVersion;
    innerFrame.device_state.name = (char *)defaultDeviceName;
    innerFrame.device_state.volume = MAX_VOLUME;

    innerFrame.device_state.can_play = true;
    innerFrame.device_state.is_active = false;
    innerFrame.device_state.has_volume = true;
    innerFrame.device_state.has_can_play = true;
    innerFrame.device_state.has_is_active = true;

    // Prepare player's capabilities
    addCapability(CapabilityType_kCanBePlayer, 1);
    addCapability(CapabilityType_kDeviceType, 4);
    addCapability(CapabilityType_kGaiaEqConnectId, 1);
    addCapability(CapabilityType_kSupportsLogout, 0);
    addCapability(CapabilityType_kIsObservable, 1);
    addCapability(CapabilityType_kVolumeSteps, 64);
    addCapability(CapabilityType_kSupportedContexts, -1,
                  std::vector<std::string>({"album", "playlist", "search", "inbox",
                                            "toplist", "starred", "publishedstarred", "track"}));
    addCapability(CapabilityType_kSupportedTypes, -1,
                  std::vector<std::string>({"audio/local", "audio/track", "local", "track"}));
    innerFrame.device_state.capabilities_count = 8;
}

void PlayerState::setPlaybackState(const PlaybackState state)
{
    switch (state)
    {
    case PlaybackState::Loading:
        // Prepare the playback at position 0
        innerFrame.state.status = PlayStatus_kPlayStatusLoading;
        innerFrame.state.position_ms = 0;
        innerFrame.state.position_measured_at = getCurrentTimestamp();
        break;
    case PlaybackState::Playing:
        innerFrame.state.status = PlayStatus_kPlayStatusPlay;
        innerFrame.state.position_measured_at = getCurrentTimestamp();
        break;
    case PlaybackState::Stopped:
        break;
    case PlaybackState::Paused:
        // Update state and recalculate current song position
        innerFrame.state.status = PlayStatus_kPlayStatusPause;
        uint32_t diff = getCurrentTimestamp() - innerFrame.state.position_measured_at;
        this->updatePositionMs(innerFrame.state.position_ms + diff);
        break;
    }
}

bool PlayerState::isActive()
{
    return innerFrame.device_state.is_active;
}

bool PlayerState::nextTrack()
{
    innerFrame.state.playing_track_index++;

    if (innerFrame.state.playing_track_index >= innerFrame.state.track_count)
    {
        innerFrame.state.playing_track_index = 0;
        setPlaybackState(PlaybackState::Paused);
        return false;
    }

    return true;
}

void PlayerState::prevTrack()
{
    if (innerFrame.state.playing_track_index > 0)
    {
        innerFrame.state.playing_track_index--;
    }
}

void PlayerState::setActive(bool isActive)
{
    innerFrame.device_state.is_active = isActive;
    if (isActive)
    {
        innerFrame.device_state.became_active_at = getCurrentTimestamp();
        innerFrame.device_state.has_became_active_at = true;
    }
}

void PlayerState::updatePositionMs(uint32_t position)
{
    innerFrame.state.position_ms = position;
    innerFrame.state.position_measured_at = getCurrentTimestamp();
}
void PlayerState::updateTracks(PBWrapper<Frame> otherFrame)
{
    innerFrame.state.context_uri = otherFrame->state.context_uri == nullptr ? nullptr : strdup(otherFrame->state.context_uri);
    printf("---- Track count %d\n", otherFrame->state.track_count);
    CSPOT_ASSERT(otherFrame->state.track_count < 100, "otherFrame->state.track_count cannot overflow track[100]");
    innerFrame.state.track_count = otherFrame->state.track_count;
    for (int i = 0; i < otherFrame->state.track_count; i++)
    {
        innerFrame.state.track[i].has_uri = otherFrame->state.track[i].has_uri;
        innerFrame.state.track[i].has_queued = otherFrame->state.track[i].has_queued;
        innerFrame.state.track[i].queued = otherFrame->state.track[i].queued;
        innerFrame.state.track[i].context = otherFrame->state.track[i].context;

        if (innerFrame.state.track[i].gid == nullptr)
        {
            free(innerFrame.state.track[i].gid);
        }

        memcpy(innerFrame.state.track[i].uri, otherFrame->state.track[i].uri, sizeof(otherFrame->state.track[i].uri));
        auto result = static_cast<pb_bytes_array_t *>(
            malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(otherFrame->state.track[i].gid->size)));
        result->size = otherFrame->state.track[i].gid->size;
        memcpy(result->bytes, otherFrame->state.track[i].gid->bytes, otherFrame->state.track[i].gid->size);
        innerFrame.state.track[i].gid = result;
    }
    innerFrame.state.has_playing_track_index = true;
    innerFrame.state.playing_track_index = otherFrame->state.playing_track_index;
}

void PlayerState::setVolume(uint32_t volume)
{
    innerFrame.device_state.volume = volume;
}

std::shared_ptr<TrackReference> PlayerState::getCurrentTrack()
{
    // Wrap current track in a class
    return std::make_shared<TrackReference>(&innerFrame.state.track[innerFrame.state.playing_track_index]);
}

std::vector<uint8_t> PlayerState::encodeCurrentFrame(MessageType typ)
{
    // Prepare current frame info
    innerFrame.version = 1;
    innerFrame.ident = (char *)deviceId;
    innerFrame.seq_nr = this->seqNum;
    innerFrame.protocol_version = (char *)protocolVersion;
    innerFrame.typ = typ;
    innerFrame.state_update_id = getCurrentTimestamp();
    innerFrame.has_version = true;
    innerFrame.has_seq_nr = true;
    innerFrame.recipient_count = 0;
    innerFrame.has_state = true;
    innerFrame.has_device_state = true;
    innerFrame.has_typ = true;
    innerFrame.has_state_update_id = true;

    this->seqNum += 1;
    return encodePB(Frame_fields, &innerFrame);
}

// Wraps messy nanopb setters. @TODO: find a better way to handle this
void PlayerState::addCapability(CapabilityType typ, int intValue, std::vector<std::string> stringValue)
{
    innerFrame.device_state.capabilities[capabilityIndex].has_typ = true;
    innerFrame.device_state.capabilities[capabilityIndex].typ = typ;

    if (intValue != -1)
    {
        innerFrame.device_state.capabilities[capabilityIndex].intValue[0] = intValue;
        innerFrame.device_state.capabilities[capabilityIndex].intValue_count = 1;
    }
    else
    {
        innerFrame.device_state.capabilities[capabilityIndex].intValue_count = 0;
    }

    for (int x = 0; x < stringValue.size(); x++)
    {
        stringValue[x].copy(innerFrame.device_state.capabilities[capabilityIndex].stringValue[x], stringValue[x].size());
        innerFrame.device_state.capabilities[capabilityIndex].stringValue[x][stringValue[x].size()] = '\0';
    }

    innerFrame.device_state.capabilities[capabilityIndex].stringValue_count = stringValue.size();
    this->capabilityIndex += 1;
}