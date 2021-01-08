// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __SYSTEM_INFOH
#define __SYSTEM_INFOH
#include <optional>
#include "CpuFamily.h"
#include "Os.h"
class SystemInfo {
public:
CpuFamily cpu_family;
Os os;
std::optional<std::string> system_information_string;
std::optional<std::string> device_id;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassSystemInfo;
};

#endif
