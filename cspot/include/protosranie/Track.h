// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __TRACKH
#define __TRACKH
#include <optional>
#include <vector>
#include "AudioFile.h"
class Track {
public:
std::optional<std::vector<uint8_t>> gid;
std::optional<std::string> name;
std::optional<int32_t> duration;
std::vector<AudioFile> file;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassTrack;
};

#endif
