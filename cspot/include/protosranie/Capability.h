// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __CAPABILITYH
#define __CAPABILITYH
#include <optional>
#include <vector>
#include "CapabilityType.h"
class Capability {
public:
std::optional<CapabilityType> typ;
std::vector<int64_t> intValue;
std::vector<std::string> stringValue;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassCapability;
};

#endif
