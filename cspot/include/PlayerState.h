#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <vector>
#include <memory>
#include <string>
#include "Utils.h"
#include "TimeProvider.h"
#include "ConstantParameters.h"
#include "CspotAssert.h"
#include "TrackReference.h"
#include "ConfigJSON.h"
#include <NanoPBHelper.h>
#include "protobuf/spirc.pb.h"

enum class PlaybackState {
    Playing,
    Stopped,
    Loading,
    Paused
};

class PlayerState
{
private:
    uint32_t seqNum = 0;
    uint8_t capabilityIndex = 0;
    std::vector<uint8_t> frameData;
    std::shared_ptr<TimeProvider> timeProvider;
    std::shared_ptr<ConfigJSON> config;

    void addCapability(CapabilityType typ, int intValue = -1, std::vector<std::string> stringsValue = std::vector<std::string>());
public:
    Frame innerFrame;
    Frame remoteFrame;

    /**
     * @brief Player state represents the current state of player.
     *
     * Responsible for keeping track of player's state. Doesn't control the playback itself.
     *
     * @param timeProvider synced time provider
     */
    PlayerState(std::shared_ptr<TimeProvider> timeProvider);
    
    ~PlayerState();

    /**
     * @brief Updates state according to current playback state.
     *
     * @param state playback state
     */
    void setPlaybackState(const PlaybackState state);

    /**
     * @brief Sets player activity
     *
     * @param isActive activity status
     */
    void setActive(bool isActive);

    /**
     * @brief Simple getter
     *
     * @return true player is active
     * @return false player is inactive
     */
    bool isActive();

    /**
     * @brief Updates local track position.
     *
     * @param position position in milliseconds
     */
    void updatePositionMs(uint32_t position);

    /**
     * @brief Sets local volume on internal state.
     *
     * @param volume volume between 0 and UINT16 max
     */
    void setVolume(uint32_t volume);

    /**
     * @brief Enables queue shuffling.
     *
     * Sets shuffle parameter on local frame, and in case shuffling is enabled,
     * it will randomize the entire local queue.
     *
     * @param shuffle whenever should shuffle
     */
    void setShuffle(bool shuffle);

    /**
     * @brief Enables repeat
     *
     * @param repeat should repeat param
     */
    void setRepeat(bool repeat);

    /**
     * @brief Updates local track queue from remote data.
     */
    void updateTracks();

    /**
     * @brief Changes playback to next queued track.
     *
     * Will go back to first track if current track is last track in queue.
     * In that case, it will pause if repeat is disabled.
     */
    bool nextTrack();

    /**
     * @brief Changes playback to previous queued track.
     *
     * Will stop if current track is the first track in queue and repeat is disabled.
     * If repeat is enabled, it will loop back to the last track in queue.
     */
    void prevTrack();

    /**
     * @brief Gets the current track reference.
     *
     * @return std::shared_ptr<TrackReference> pointer to track reference
     */
    std::shared_ptr<TrackReference> getCurrentTrack();

    /**
     * @brief Encodes current frame into binary data via protobuf.
     *
     * @param typ message type to include in frame type
     * @return std::vector<uint8_t> binary frame data
     */
    std::vector<uint8_t> encodeCurrentFrame(MessageType typ);
};

#endif
