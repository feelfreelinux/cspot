#pragma once

#include <stdint.h>   // for uint8_t, uint32_t
#include <algorithm>  //for sort
#include <memory>     // for shared_ptr
#include <string>     // for string
#include <vector>     // for vector

namespace cspot {
struct Context;
class QueuedTrack;

struct TrackInterval {
  uint64_t start;
  uint64_t end;
  uint64_t position;
  uint64_t length;

  TrackInterval();
  TrackInterval(uint64_t start, uint64_t position) {
    this->start = start;
    this->position = position;
  }
};

struct skip {
  uint8_t count = 0;
  uint64_t amount = 0;

  void add(uint64_t amount) {
    this->amount += amount;
    count++;
  }
};

class TrackMetrics {
 public:
  TrackMetrics(std::shared_ptr<cspot::Context> ctx, uint64_t pos = 0);

  std::shared_ptr<TrackInterval> currentInterval;
  uint64_t longestInterval = 0, totalAmountOfPlayTime = 0,
           totalMsOfPlayedTrack = 0, totalAmountPlayed = 0, audioKeyTime = 0,
           trackHeaderTime = 0, written_bytes = 0, track_size = 0,
           timestamp = 0;
  std::vector<std::pair<uint64_t, uint64_t>> intervals;
  skip skipped_backward, skipped_forward;

  void newPosition(uint64_t pos);
  void endInterval(uint64_t pos);
  void endTrack();
  void startTrackDecoding();
  void startTrack(uint64_t pos);
  uint64_t getPosition();

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::vector<std::pair<uint64_t, uint64_t>> addInterval(
      std::vector<std::pair<uint64_t, uint64_t>>& intervals,
      std::pair<std::uint64_t, std::uint64_t> newInterval) {
    // Add the new interval to the list of intervals
    intervals.push_back(newInterval);

    // Sort intervals by starting time
    std::sort(intervals.begin(), intervals.end());

    // Result vector to hold merged intervals
    std::vector<std::pair<uint64_t, uint64_t>> merged;

    // Iterate over intervals to merge overlapping ones
    for (const auto& interval : intervals) {
      // If merged is empty or current interval does not overlap with the previous one
      if (merged.empty() || merged.back().second < interval.first) {
        merged.push_back(interval);
      } else {
        // If it overlaps, merge the intervals
        merged.back().second = std::max(merged.back().second, interval.second);
      }
    }

    return merged;
  }
};
class PlaybackMetrics {
 private:
  std::shared_ptr<cspot::Context> ctx;
  uint32_t seqNum = 0;
  size_t timestamp;

  std::string get_source_from_context();
  std::string get_end_source();

  void append(std::vector<uint8_t>* data, std::string to_do) {
    data->insert(data->end(), to_do.begin(), to_do.end());
    data->push_back(9);
  }

 public:
  PlaybackMetrics(std::shared_ptr<cspot::Context> ctx) { this->ctx = ctx; }
  enum reason {
    TRACK_DONE,
    TRACK_ERROR,
    FORWARD_BTN,
    BACKWARD_BTN,
    END_PLAY,
    PLAY_BTN,
    CLICK_ROW,
    LOGOUT,
    APP_LOAD,
    REMOTE,
    UNKNOWN
  } end_reason = UNKNOWN,
    start_reason = UNKNOWN, nested_reason = UNKNOWN;

  std::string context_uri, correlation_id, start_source, end_source;
  std::shared_ptr<cspot::TrackMetrics> trackMetrics;

  std::vector<uint8_t> sendEvent(std::shared_ptr<cspot::QueuedTrack> track);
};
}  // namespace cspot