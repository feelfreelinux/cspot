#ifndef PBWIRETYPE_H
#define PBWIRETYPE_H

#include <cstdint>

enum class PbWireType : uint32_t
{
    varint = 0,           // int32/64, uint32/64, sint32/64, bool, enum
    fixed64 = 1,          // fixed64, sfixed64, double
    length_delimited = 2, // string, bytes, nested messages, packed repeated fields
    fixed32 = 5,          // fixed32, sfixed32, float
    unknown = 99
};

#endif