#ifndef PBREADER_H
#define PBREADER_H

#include <vector>
#include <string>
#include <memory>
#include <PbWireType.h>

class PbReader
{
private:
    std::vector<uint8_t> const &rawData;
    uint32_t currentWireValue = 0;
    uint64_t skipVarIntDump = 0;
    uint32_t nextFieldLength = 0;
    int64_t decodeZigzag(uint64_t value);

public:
    PbReader(std::vector<uint8_t> const &rawData);
    uint32_t maxPosition = 0;

    PbWireType currentWireType = PbWireType::unknown;
    uint32_t currentTag = 0;
    uint32_t pos = 0;

    template <typename T>
    T decodeVarInt();

    template <typename T>
    T decodeFixed();

    template <typename T>
    T decodeSVarInt();
    void decodeString(std::string &target);
    void decodeVector(std::vector<uint8_t> &target);

    bool next();
    void skip();
    void resetMaxPosition();
};

#endif