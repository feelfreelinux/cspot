// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __DEVICE_STATEH
#define __DEVICE_STATEH
#include <optional>
#include <vector>
#include "Capability.h"
class DeviceState {
public:
std::optional<std::string> sw_version;
std::optional<bool> is_active;
std::optional<bool> can_play;
std::optional<uint32_t> volume;
std::optional<std::string> name;
std::optional<uint32_t> error_code;
std::optional<int64_t> became_active_at;
std::optional<std::string> error_message;
std::vector<Capability> capabilities;
std::vector<std::string> local_uris;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassDeviceState;
};

#endif
