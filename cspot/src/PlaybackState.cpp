#include "PlaybackState.h"
#include <memory>
#include "CSpotContext.h"
#include "Logger.h"

using namespace cspot;

PlaybackState::PlaybackState(std::shared_ptr<cspot::Context> ctx)
{
    this->ctx = ctx;
    innerFrame = {};
    remoteFrame = {};

    // Prepare default state
    innerFrame.state.has_position_ms = true;
    innerFrame.state.position_ms = 0;

    innerFrame.state.status = PlayStatus_kPlayStatusStop;
    innerFrame.state.has_status = true;

    innerFrame.state.position_measured_at = 0;
    innerFrame.state.has_position_measured_at = true;

    innerFrame.state.shuffle = false;
    innerFrame.state.has_shuffle = true;

    innerFrame.state.repeat = false;
    innerFrame.state.has_repeat = true;

    innerFrame.device_state.sw_version = strdup(swVersion);

    innerFrame.device_state.is_active = false;
    innerFrame.device_state.has_is_active = true;

    innerFrame.device_state.can_play = true;
    innerFrame.device_state.has_can_play = true;

    innerFrame.device_state.volume = ctx->config.volume;
    innerFrame.device_state.has_volume = true;

    innerFrame.device_state.name = strdup(ctx->config.deviceName.c_str());

    innerFrame.state.track_count = 0;

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
                  std::vector<std::string>({"audio/local", "audio/track", "audio/episode", "local", "track"}));
    innerFrame.device_state.capabilities_count = 8;
}

PlaybackState::~PlaybackState() {
    pb_release(Frame_fields, &innerFrame);
    pb_release(Frame_fields, &remoteFrame);
}

void PlaybackState::setPlaybackState(const PlaybackState::State state)
{
    switch (state)
    {
        case State::Loading:
            // Prepare the playback at position 0
            innerFrame.state.status = PlayStatus_kPlayStatusPause;
            innerFrame.state.position_ms = 0;
            innerFrame.state.position_measured_at = ctx->timeProvider->getSyncedTimestamp();
            break;
        case State::Playing:
            innerFrame.state.status = PlayStatus_kPlayStatusPlay;
            innerFrame.state.position_measured_at = ctx->timeProvider->getSyncedTimestamp();
            break;
        case State::Stopped:
            break;
        case State::Paused:
            // Update state and recalculate current song position
            innerFrame.state.status = PlayStatus_kPlayStatusPause;
            uint32_t diff = ctx->timeProvider->getSyncedTimestamp() - innerFrame.state.position_measured_at;
            this->updatePositionMs(innerFrame.state.position_ms + diff);
            break;
    }
}

bool PlaybackState::isActive()
{
    return innerFrame.device_state.is_active;
}

bool PlaybackState::nextTrack()
{
    if (innerFrame.state.repeat) return true;

    innerFrame.state.playing_track_index++;

    if (innerFrame.state.playing_track_index >= innerFrame.state.track_count)
    {
        innerFrame.state.playing_track_index = 0;
        if (!innerFrame.state.repeat)
        {
            setPlaybackState(State::Paused);
            return false;
        }
    }

    return true;
}

void PlaybackState::prevTrack()
{
    if (innerFrame.state.playing_track_index > 0)
    {
        innerFrame.state.playing_track_index--;
    }
    else if (innerFrame.state.repeat)
    {
        innerFrame.state.playing_track_index = innerFrame.state.track_count - 1;
    }
}

void PlaybackState::setActive(bool isActive)
{
    innerFrame.device_state.is_active = isActive;
    if (isActive)
    {
        innerFrame.device_state.became_active_at = ctx->timeProvider->getSyncedTimestamp();
        innerFrame.device_state.has_became_active_at = true;
    }
}

void PlaybackState::updatePositionMs(uint32_t position)
{
    innerFrame.state.position_ms = position;
    innerFrame.state.position_measured_at = ctx->timeProvider->getSyncedTimestamp();
}

#define FREE(ptr) { free(ptr); ptr = NULL; }
#define STRDUP(dst, src) if(src != NULL) { dst = strdup(src); } else { FREE(dst); } // strdup null pointer safe

void PlaybackState::updateTracks()
{
    CSPOT_LOG(info, "---- Track count %d", remoteFrame.state.track_count);
    CSPOT_LOG(info, "---- Inner track count %d", innerFrame.state.track_count);
    CSPOT_LOG(info, "--- Context URI %s", remoteFrame.state.context_uri);

    // free unused tracks
    if(innerFrame.state.track_count > remoteFrame.state.track_count)
    {
        for(uint16_t i = remoteFrame.state.track_count; i < innerFrame.state.track_count; ++i)
        {
            FREE(innerFrame.state.track[i].gid);
            FREE(innerFrame.state.track[i].uri);
            FREE(innerFrame.state.track[i].context);
        }
    }
    
    // reallocate memory for new tracks
    innerFrame.state.track = (TrackRef *) realloc(innerFrame.state.track, sizeof(TrackRef) * remoteFrame.state.track_count);

    for(uint16_t i = 0; i < remoteFrame.state.track_count; ++i)
    {
        if(i >= innerFrame.state.track_count) {
            innerFrame.state.track[i].gid = NULL;
            innerFrame.state.track[i].uri = NULL;
            innerFrame.state.track[i].context = NULL;
        }

        if(remoteFrame.state.track[i].gid != NULL)
        {
            uint16_t gid_size = remoteFrame.state.track[i].gid->size;
            innerFrame.state.track[i].gid = (pb_bytes_array_t *) realloc(innerFrame.state.track[i].gid, PB_BYTES_ARRAY_T_ALLOCSIZE(gid_size));
            
            memcpy(innerFrame.state.track[i].gid->bytes, remoteFrame.state.track[i].gid->bytes, gid_size);
            innerFrame.state.track[i].gid->size = gid_size;
        }
        innerFrame.state.track[i].has_queued = remoteFrame.state.track[i].has_queued;
        innerFrame.state.track[i].queued = remoteFrame.state.track[i].queued;

        STRDUP(innerFrame.state.track[i].uri, remoteFrame.state.track[i].uri);
        STRDUP(innerFrame.state.track[i].context, remoteFrame.state.track[i].context);
    }

    innerFrame.state.context_uri = (char *) realloc(innerFrame.state.context_uri,
                                            strlen(remoteFrame.state.context_uri) + 1);
    strcpy(innerFrame.state.context_uri, remoteFrame.state.context_uri);

    innerFrame.state.track_count = remoteFrame.state.track_count;
    innerFrame.state.has_playing_track_index = true;
    innerFrame.state.playing_track_index = remoteFrame.state.playing_track_index;

    if (remoteFrame.state.repeat)
    {
        setRepeat(true);
    }

    if (remoteFrame.state.shuffle)
    {
        setShuffle(true);
    }
}

void PlaybackState::setVolume(uint32_t volume)
{
    innerFrame.device_state.volume = volume;
    ctx->config.volume = volume;
}

void PlaybackState::setShuffle(bool shuffle)
{
    innerFrame.state.shuffle = shuffle;
    if (shuffle)
    {
        // Put current song at the begining
        std::swap(innerFrame.state.track[0], innerFrame.state.track[innerFrame.state.playing_track_index]);

        // Shuffle current tracks
        for (int x = 1; x < innerFrame.state.track_count - 1; x++)
        {
            auto j = x + (std::rand() % (innerFrame.state.track_count - x));
            std::swap(innerFrame.state.track[j], innerFrame.state.track[x]);
        }
        innerFrame.state.playing_track_index = 0;
    }
}

void PlaybackState::setRepeat(bool repeat)
{
    innerFrame.state.repeat = repeat;
}

TrackRef* PlaybackState::getCurrentTrackRef()
{
    return &innerFrame.state.track[innerFrame.state.playing_track_index];
}

TrackRef* PlaybackState::getNextTrackRef()
{
    if (innerFrame.state.playing_track_index >= innerFrame.state.track_count) {
        return nullptr;
    }

    return &innerFrame.state.track[innerFrame.state.playing_track_index + 1];
}

std::vector<uint8_t> PlaybackState::encodeCurrentFrame(MessageType typ)
{
    free(innerFrame.ident);
    free(innerFrame.protocol_version);

    // Prepare current frame info
    innerFrame.version = 1;
    innerFrame.ident = strdup(ctx->config.deviceId.c_str());
    innerFrame.seq_nr = this->seqNum;
    innerFrame.protocol_version = strdup(protocolVersion);
    innerFrame.typ = typ;
    innerFrame.state_update_id = ctx->timeProvider->getSyncedTimestamp();
    innerFrame.has_version = true;
    innerFrame.has_seq_nr = true;
    innerFrame.recipient_count = 0;
    innerFrame.has_state = true;
    innerFrame.has_device_state = true;
    innerFrame.has_typ = true;
    innerFrame.has_state_update_id = true;

    this->seqNum += 1;
    return pbEncode(Frame_fields, &innerFrame);
}

// Wraps messy nanopb setters. @TODO: find a better way to handle this
void PlaybackState::addCapability(CapabilityType typ, int intValue, std::vector<std::string> stringValue)
{
    innerFrame.device_state.capabilities[capabilityIndex].has_typ = true;
    this->innerFrame.device_state.capabilities[capabilityIndex].typ = typ;

    if (intValue != -1)
    {
        this->innerFrame.device_state.capabilities[capabilityIndex].intValue[0] = intValue;
        this->innerFrame.device_state.capabilities[capabilityIndex].intValue_count = 1;
    }
    else
    {
        this->innerFrame.device_state.capabilities[capabilityIndex].intValue_count = 0;
    }

    for (int x = 0; x < stringValue.size(); x++)
    {
        pbPutString(stringValue[x], this->innerFrame.device_state.capabilities[capabilityIndex].stringValue[x]);
    }

    this->innerFrame.device_state.capabilities[capabilityIndex].stringValue_count = stringValue.size();
    this->capabilityIndex += 1;
}
