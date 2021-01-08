// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __FRAMEH
#define __FRAMEH
#include <optional>
#include <vector>
#include "MessageType.h"
#include "DeviceState.h"
#include "State.h"
class Frame {
public:
std::optional<uint32_t> version;
std::optional<std::string> ident;
std::optional<std::string> protocol_version;
std::optional<uint32_t> seq_nr;
std::optional<MessageType> typ;
std::optional<DeviceState> device_state;
std::optional<State> state;
std::optional<uint32_t> position;
std::optional<uint32_t> volume;
std::optional<int64_t> state_update_id;
std::vector<std::string> recipient;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassFrame;
};

#endif
