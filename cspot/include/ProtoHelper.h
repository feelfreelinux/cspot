#ifndef PROTOBUF_H
#define PROTOBUF_H
#include <iostream>
#include <memory>
#include "protobuf.h"
#include <PbReader.h>
#include <PbCommon.h>

std::optional<AnyRef> findFieldWithProtobufTag(AnyRef ref, uint32_t tag);
void decodeField(std::shared_ptr<PbReader> reader, AnyRef any);
void decodeProtobuf(std::shared_ptr<PbReader> reader, AnyRef any);
void encodeProtobuf(std::shared_ptr<PbWriter> writer, AnyRef any, uint32_t protobufTag = 0);

template <typename T>
std::vector<uint8_t> encodePb(T & data)
{
    auto ref = AnyRef::of(&data);
    std::vector<uint8_t> rawData;;
    auto writer = std::make_shared<PbWriter>(rawData);
    encodeProtobuf(writer, ref);

     return rawData;
}


template <typename T>
T decodePb(std::vector<uint8_t> & bytes)
{
    T data = {};
    auto ref = AnyRef::of(&data);
    auto writer = std::make_shared<PbReader>(bytes);
    decodeProtobuf(writer, ref);

     return data;
}

#endif