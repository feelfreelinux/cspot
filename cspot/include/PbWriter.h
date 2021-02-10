#ifndef PBWRITER_H
#define PBWRITER_H

#include <vector>
#include <string>
#include <memory>
#include <PbWireType.h>

class PbWriter
{
private:
    std::vector<uint8_t> &rawData;
    uint32_t pos;
    uint32_t msgStartPos = 0;
    void encodeVarInt(uint32_t low, uint32_t high, int32_t atIndex = 0);
    uint32_t encodeZigzag32(int32_t value);
    uint64_t encodeZigzag64(int64_t value);
public:
    PbWriter(std::vector<uint8_t> &rawData);

    template <typename T>
    void encodeVarInt(T, int32_t atIndex = 0);
    template <typename T>
    void encodeFixed(T);
    void addSVarInt32(uint32_t tag, int32_t);
    void addSVarInt64(uint32_t tag, int64_t);
    void addString(uint32_t tag, std::string &target);
    void addVector(uint32_t tag, std::vector<uint8_t> &target);

    template <typename T>
    void addVarInt(uint32_t tag, T intType);
    void addBool(uint32_t tag, bool value);

    void addField(uint32_t tag, PbWireType wiretype);
    uint32_t startMessage();
    void finishMessage(uint32_t tag, uint32_t pos);
};

#endif