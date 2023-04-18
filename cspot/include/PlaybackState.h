#pragma once

#include <stdint.h>             // for uint8_t, uint32_t
#include <memory>               // for shared_ptr
#include <string>               // for string
#include <vector>               // for vector

#include "protobuf/spirc.pb.h"  // for Frame, TrackRef, CapabilityType, Mess...

namespace cspot {
struct Context;

class PlaybackState {
 private:
  std::shared_ptr<cspot::Context> ctx;
  uint32_t seqNum = 0;
  uint8_t capabilityIndex = 0;
  std::vector<uint8_t> frameData;

  void addCapability(
      CapabilityType typ, int intValue = -1,
      std::vector<std::string> stringsValue = std::vector<std::string>());

 public:
  Frame innerFrame;
  Frame remoteFrame;
  enum class State { Playing, Stopped, Loading, Paused };

  /**
     * @brief Player state represents the current state of player.
     *
     * Responsible for keeping track of player's state. Doesn't control the playback itself.
     *
     * @param timeProvider synced time provider
     */
  PlaybackState(std::shared_ptr<cspot::Context> ctx);

  ~PlaybackState();

  /**
     * @brief Updates state according to current playback state.
     *
     * @param state playback state
     */
  void setPlaybackState(const PlaybackState::State state);

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
  TrackRef* getCurrentTrackRef();

  /**
    * @brief Gets reference to next track in queue, or nullptr if there is no next track.
    *
    * @return std::shared_ptr<TrackReference> pointer to track reference
    */
  TrackRef* getNextTrackRef();

  /**
     * @brief Encodes current frame into binary data via protobuf.
     *
     * @param typ message type to include in frame type
     * @return std::vector<uint8_t> binary frame data
     */
  std::vector<uint8_t> encodeCurrentFrame(MessageType typ);
};
}  // namespace cspot