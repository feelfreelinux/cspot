#include "PlayerState.h"
#include "Logger.h"

PlayerState::PlayerState(std::shared_ptr<TimeProvider> timeProvider)
{
    this->timeProvider = timeProvider;

    // Prepare default state
    innerFrame.state.emplace();
    innerFrame.state->position_ms = 0;
    innerFrame.state->status = PlayStatus::kPlayStatusStop;
    innerFrame.state->position_measured_at = 0;
    innerFrame.state->shuffle = false;
    innerFrame.state->repeat = false;

    innerFrame.device_state.emplace();
    innerFrame.device_state->sw_version = swVersion;
    innerFrame.device_state->is_active = false;
    innerFrame.device_state->can_play = true;
    innerFrame.device_state->volume = MAX_VOLUME;
    innerFrame.device_state->name = defaultDeviceName;

    // Prepare player's capabilities
    innerFrame.device_state->capabilities = std::vector<Capability>();
    addCapability(CapabilityType::kCanBePlayer, 1);
    addCapability(CapabilityType::kDeviceType, 4);
    addCapability(CapabilityType::kGaiaEqConnectId, 1);
    addCapability(CapabilityType::kSupportsLogout, 0);
    addCapability(CapabilityType::kIsObservable, 1);
    addCapability(CapabilityType::kVolumeSteps, 64);
    addCapability(CapabilityType::kSupportedContexts, -1,
                  std::vector<std::string>({"album", "playlist", "search", "inbox",
                                            "toplist", "starred", "publishedstarred", "track"}));
    addCapability(CapabilityType::kSupportedTypes, -1,
                  std::vector<std::string>({"audio/local", "audio/track", "audio/episode", "local", "track"}));
}

void PlayerState::setPlaybackState(const PlaybackState state)
{
    switch (state)
    {
    case PlaybackState::Loading:
        // Prepare the playback at position 0
        innerFrame.state->status = PlayStatus::kPlayStatusLoading;
        innerFrame.state->position_ms = 0;
        innerFrame.state->position_measured_at = timeProvider->getSyncedTimestamp();
        break;
    case PlaybackState::Playing:
        innerFrame.state->status = PlayStatus::kPlayStatusPlay;
        innerFrame.state->position_measured_at = timeProvider->getSyncedTimestamp();
        break;
    case PlaybackState::Stopped:
        break;
    case PlaybackState::Paused:
        // Update state and recalculate current song position
        innerFrame.state->status = PlayStatus::kPlayStatusPause;
        uint32_t diff = timeProvider->getSyncedTimestamp() - innerFrame.state->position_measured_at.value();
        this->updatePositionMs(innerFrame.state->position_ms.value() + diff);
        break;
    }
}

bool PlayerState::isActive()
{
    return innerFrame.device_state->is_active.value();
}

bool PlayerState::nextTrack()
{
    innerFrame.state->playing_track_index.value()++;

    if (innerFrame.state->playing_track_index >= innerFrame.state->track.size())
    {
        innerFrame.state->playing_track_index = 0;
        if (!innerFrame.state->repeat)
        {
            setPlaybackState(PlaybackState::Paused);
            return false;
        }
    }

    return true;
}

void PlayerState::prevTrack()
{
    if (innerFrame.state->playing_track_index > 0)
    {
        innerFrame.state->playing_track_index.value()--;
    }
    else if (innerFrame.state->repeat)
    {
        innerFrame.state->playing_track_index = innerFrame.state->track.size() - 1;
    }
}

void PlayerState::setActive(bool isActive)
{
    innerFrame.device_state->is_active = isActive;
    if (isActive)
    {
        innerFrame.device_state->became_active_at = timeProvider->getSyncedTimestamp();
    }
}

void PlayerState::updatePositionMs(uint32_t position)
{
    innerFrame.state->position_ms = position;
    innerFrame.state->position_measured_at = timeProvider->getSyncedTimestamp();
}
void PlayerState::updateTracks()
{
    CSPOT_LOG(info, "---- Track count %d", remoteFrame.state->track.size());
    // innerFrame.state->context_uri = remoteFrame.state->context_uri == nullptr ? nullptr : strdup(otherFrame->state->context_uri);

    innerFrame.state->track = remoteFrame.state->track;
    innerFrame.state->playing_track_index = remoteFrame.state->playing_track_index;

    if (remoteFrame.state->repeat.value())
    {
        setRepeat(true);
    }

    if (remoteFrame.state->shuffle.value())
    {
        setShuffle(true);
    }
}

void PlayerState::setVolume(uint32_t volume)
{
    innerFrame.device_state->volume = volume;
}

void PlayerState::setShuffle(bool shuffle)
{
    innerFrame.state->shuffle = shuffle;
    if (shuffle)
    {
        // Put current song at the begining
        auto tmp = innerFrame.state->track.at(0);
        innerFrame.state->track.at(0) = innerFrame.state->track.at(innerFrame.state->playing_track_index.value());
        innerFrame.state->track.at(innerFrame.state->playing_track_index.value()) = tmp;

        // Shuffle current tracks
        for (int x = 1; x < innerFrame.state->track.size() - 1; x++)
        {
            auto j = x + (std::rand() % (innerFrame.state->track.size() - x));
            tmp = innerFrame.state->track.at(j);
            innerFrame.state->track.at(j) = innerFrame.state->track.at(x);
            innerFrame.state->track.at(x) = tmp;
        }
        innerFrame.state->playing_track_index = 0;
    }
}

void PlayerState::setRepeat(bool repeat)
{
    innerFrame.state->repeat = repeat;
}

std::shared_ptr<TrackReference> PlayerState::getCurrentTrack()
{
    // Wrap current track in a class
    return std::make_shared<TrackReference>(&innerFrame.state->track.at(innerFrame.state->playing_track_index.value()));
}

std::vector<uint8_t> PlayerState::encodeCurrentFrame(MessageType typ)
{
    // Prepare current frame info
    innerFrame.version = 1;
    innerFrame.ident = deviceId;
    innerFrame.seq_nr = this->seqNum;
    innerFrame.protocol_version = protocolVersion;
    innerFrame.typ = typ;
    innerFrame.state_update_id = timeProvider->getSyncedTimestamp();

    this->seqNum += 1;
    auto fram = encodePb(innerFrame);
    return fram;
}

// Wraps messy nanopb setters. @TODO: find a better way to handle this
void PlayerState::addCapability(CapabilityType typ, int intValue, std::vector<std::string> stringValue)
{
    auto capability = Capability();
    capability.typ = typ;

    if (intValue != -1)
    {
        capability.intValue = std::vector<int64_t>({intValue});
    }

    capability.stringValue = stringValue;
    innerFrame.device_state->capabilities.push_back(capability);
}
