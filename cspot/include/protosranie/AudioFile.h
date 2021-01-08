// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __AUDIO_FILEH
#define __AUDIO_FILEH
#include <optional>
#include <vector>
#include "AudioFormat.h"
class AudioFile {
public:
std::optional<std::vector<uint8_t>> file_id;
std::optional<AudioFormat> format;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassAudioFile;
};

#endif
