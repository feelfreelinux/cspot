#include "PbReader.h"
#include <iostream>

PbReader::PbReader(std::vector<uint8_t> const &rawData) : rawData(rawData)
{
    maxPosition = rawData.size();
}

template <typename T>
T PbReader::decodeVarInt()
{
    uint8_t byte;
    uint_fast8_t bitpos = 0;
    uint64_t storago = 0;
    do
    {
        byte = this->rawData[pos];
        pos++;

        storago |= (uint64_t)(byte & 0x7F) << bitpos;
        bitpos = (uint_fast8_t)(bitpos + 7);
    } while (byte & 0x80);
    return static_cast<T>(storago);
}

template <typename T>
T PbReader::decodeFixed()
{
    pos += sizeof(T);
    return *(T*)(&this->rawData[pos - sizeof(T)]);
}


template int32_t PbReader::decodeFixed();
template int64_t PbReader::decodeFixed();

template uint32_t PbReader::decodeVarInt();
template int64_t PbReader::decodeVarInt();
template bool PbReader::decodeVarInt();

void PbReader::resetMaxPosition()
{
    maxPosition = rawData.size();
}

void PbReader::decodeString(std::string &target)
{
    nextFieldLength = decodeVarInt<uint32_t>();
    target.resize(nextFieldLength);
    // std::cout << "rawData.size() = " << rawData.size() << " pos = " << pos << " nextFieldLength =" << nextFieldLength;
    // printf("\n%d, \n", currentTag);
    // if (pos + nextFieldLength >= rawData.size())
    // {
    //     std::cout << " \nBAD --  pos + nextFieldLength >= rawData.size()  MSVC IS LITERLALLY SHAKING AND CRYING RN";
    // }
    // std::cout << std::endl;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + nextFieldLength, target.begin());
    pos += nextFieldLength;
}

void PbReader::decodeVector(std::vector<uint8_t> &target)
{
    nextFieldLength = decodeVarInt<uint32_t>();
    target.resize(nextFieldLength);
    std::copy(rawData.begin() + pos, rawData.begin() + pos + nextFieldLength, target.begin());
    pos += nextFieldLength;
}

bool PbReader::next()
{
    if (pos >= maxPosition)
        return false;

    currentWireValue = decodeVarInt<uint32_t>();
    currentTag = currentWireValue >> 3U;
    currentWireType = PbWireType(currentWireValue & 0x07U);
    return true;
}

int64_t PbReader::decodeZigzag(uint64_t value)
{
    return static_cast<int64_t>((value >> 1U) ^ static_cast<uint64_t>(-static_cast<int64_t>(value & 1U)));
}

template <typename T>
T PbReader::decodeSVarInt()
{
    skipVarIntDump = decodeVarInt<uint64_t>();
    return static_cast<T>(decodeZigzag(skipVarIntDump));
}

template int32_t PbReader::decodeSVarInt();
template int64_t PbReader::decodeSVarInt();

void PbReader::skip()
{
    switch (currentWireType)
    {
    case PbWireType::varint:
        skipVarIntDump = decodeVarInt<uint64_t>();
        break;
    case PbWireType::fixed64:
        pos += 8;
        break;
    case PbWireType::length_delimited:
        nextFieldLength = decodeVarInt<uint32_t>();
        pos += nextFieldLength;
        break;
    case PbWireType::fixed32:
        pos += 4;
        break;
    default:
        break;
    }
}
