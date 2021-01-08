// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __STATEH
#define __STATEH
#include <optional>
#include <vector>
#include "PlayStatus.h"
#include "TrackRef.h"
class State {
public:
std::optional<std::string> context_uri;
std::optional<uint32_t> index;
std::optional<uint32_t> position_ms;
std::optional<PlayStatus> status;
std::optional<uint64_t> position_measured_at;
std::optional<std::string> context_description;
std::optional<bool> shuffle;
std::optional<bool> repeat;
std::optional<uint32_t> playing_track_index;
std::vector<TrackRef> track;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassState;
};

#endif
