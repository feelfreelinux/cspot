#include "PbWriter.h"

PbWriter::PbWriter(std::vector<uint8_t> &rawData) : rawData(rawData)
{
}

void PbWriter::encodeVarInt(uint32_t low, uint32_t high, int32_t indexOffset)
{
    size_t i = 0;
    uint8_t byte = (uint8_t)(low & 0x7F);
    low >>= 7;

    while (i < 4 && (low != 0 || high != 0))
    {
        byte |= 0x80;
        rawData.insert(rawData.end() + indexOffset, byte);
        i++;
        byte = (uint8_t)(low & 0x7F);
        low >>= 7;
    }

    if (high)
    {
        byte = (uint8_t)(byte | ((high & 0x07) << 4));
        high >>= 3;

        while (high)
        {
            byte |= 0x80;
            rawData.insert(rawData.end() + indexOffset, byte);
            i++;
            byte = (uint8_t)(high & 0x7F);
            high >>= 7;
        }
    }

    rawData.insert(rawData.end() + indexOffset, byte);
}

template void PbWriter::encodeVarInt(uint8_t, int32_t);
template void PbWriter::encodeVarInt(uint32_t, int32_t);
template void PbWriter::encodeVarInt(uint64_t, int32_t);
template void PbWriter::encodeVarInt(long long, int32_t);

template <typename T>
void PbWriter::encodeVarInt(T data, int32_t offset)
{
    auto value = static_cast<uint64_t>(data);
    if (value <= 0x7F)
    {
        rawData.insert(rawData.end() + offset, (uint8_t)value);
    }
    else
    {
        encodeVarInt((uint32_t)value, (uint32_t)(value >> 32), offset);
    }
}

uint32_t PbWriter::encodeZigzag32(int32_t value) {
    return (static_cast<uint32_t>(value) << 1U) ^ static_cast<uint32_t>(-static_cast<int32_t>(static_cast<uint32_t>(value) >> 31U));
}

uint64_t PbWriter::encodeZigzag64(int64_t value) {
    return (static_cast<uint64_t>(value) << 1U) ^ static_cast<uint64_t>(-static_cast<int64_t>(static_cast<uint64_t>(value) >> 63U));
}

void PbWriter::addSVarInt32(uint32_t tag, int32_t data) {
    auto val = encodeZigzag32(data);
    addVarInt(tag, val);
}

template <typename T>
void PbWriter::encodeFixed(T data) {
    auto val = reinterpret_cast<const char*>(&data);
    rawData.insert(rawData.end(), val, val + sizeof(T));
}

template void PbWriter::encodeFixed(int64_t);
template void PbWriter::encodeFixed(int32_t);

void PbWriter::addSVarInt64(uint32_t tag, int64_t data) {
    auto val = encodeZigzag64(data);
    addVarInt(tag, val);
}

void PbWriter::addString(uint32_t tag, std::string &target)
{
    addField(tag, PbWireType::length_delimited);
    uint32_t stringSize = target.size();
    encodeVarInt(stringSize);

    rawData.insert(rawData.end(), target.begin(), target.end());
}

void PbWriter::addVector(uint32_t tag, std::vector<uint8_t> &target)
{
    addField(tag, PbWireType::length_delimited);
    uint32_t vectorSize = target.size();
    encodeVarInt(vectorSize);

    rawData.insert(rawData.end(), target.begin(), target.end());
}

template <typename T>
void PbWriter::addVarInt(uint32_t tag, T intType)
{
    addField(tag, PbWireType::varint);
    encodeVarInt(intType);
}

void PbWriter::addBool(uint32_t tag, bool value)
{
    addField(tag, PbWireType::varint);
    rawData.push_back(char(value));
}

template void PbWriter::addVarInt(uint32_t, uint8_t);
template void PbWriter::addVarInt(uint32_t, uint32_t);
template void PbWriter::addVarInt(uint32_t, uint64_t);
template void PbWriter::addVarInt(uint32_t, int64_t);
template void PbWriter::addVarInt(uint32_t, bool);

void PbWriter::addField(uint32_t tag, PbWireType wiretype)
{
    const uint32_t value = (tag << 3U) | uint32_t(wiretype);
    encodeVarInt(value);
}

uint32_t PbWriter::startMessage()
{
    return rawData.size();
}

void PbWriter::finishMessage(uint32_t tag, uint32_t lastMessagePosition)
{
    uint32_t finalMessageSize = rawData.size() - lastMessagePosition;
    uint32_t msgHeader = (tag << 3U) | uint32_t(PbWireType::length_delimited);

    int32_t offset = -finalMessageSize;
    encodeVarInt(msgHeader, offset);
    encodeVarInt(finalMessageSize, offset);
}
