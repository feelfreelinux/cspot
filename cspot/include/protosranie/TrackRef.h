// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __TRACK_REFH
#define __TRACK_REFH
#include <optional>
#include <vector>
class TrackRef {
public:
std::optional<std::vector<uint8_t>> gid;
std::optional<std::string> uri;
std::optional<bool> queued;
std::optional<std::string> context;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassTrackRef;
};

#endif
