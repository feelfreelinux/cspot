#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <vector>
#include <memory>
#include <string>
#include "spirc.pb.h"
#include "Utils.h"
#include "ConstantParameters.h"
#include "PBUtils.h"
#include "CspotAssert.h"
#include "TrackReference.h"

enum class PlaybackState {
    Playing,
    Stopped,
    Loading,
    Paused
};

class PlayerState
{
private:
    Frame innerFrame;
    uint32_t seqNum = 0;
    uint8_t capabilityIndex = 0;

    void addCapability(CapabilityType typ, int intValue = -1, std::vector<std::string> stringsValue = std::vector<std::string>());
public:
    PlayerState();
    void setPlaybackState(const PlaybackState state);
    void setActive(bool isActive);
    bool isActive();
    void updatePositionMs(uint32_t position);
    void setVolume(uint32_t volume);
    void updateTracks(PBWrapper<Frame> otherFrame);
    bool nextTrack();
    void prevTrack();
    std::shared_ptr<TrackReference> getCurrentTrack();

    std::vector<uint8_t> encodeCurrentFrame(MessageType typ);
};

#endif