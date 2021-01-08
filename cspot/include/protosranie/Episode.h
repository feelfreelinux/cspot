// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __EPISODEH
#define __EPISODEH
#include <optional>
#include <vector>
#include "AudioFile.h"
class Episode {
public:
std::optional<std::vector<uint8_t>> gid;
std::optional<std::string> name;
std::optional<int32_t> duration;
std::vector<AudioFile> audio;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassEpisode;
};

#endif
