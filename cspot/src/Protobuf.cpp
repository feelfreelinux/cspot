#include "ProtoHelper.h"


std::optional<AnyRef> findFieldWithProtobufTag(AnyRef ref, uint32_t tag)
{
    auto info = ref.reflectType();
    for (int i = 0; i < info->fields.size(); i++)
    {

        if (tag == info->fields[i].protobufTag)
        {
            return std::make_optional(ref.getField(i));
        }
    }

    return std::nullopt;
}

void decodeField(std::shared_ptr<PbReader> reader, AnyRef any)
{
    auto fieldInfo = any.reflectType();

    if (fieldInfo->kind == ReflectTypeKind::Optional)
    {
        auto optionalRef = AnyOptionalRef(any);
        optionalRef.emplaceEmpty();
        return decodeField(reader, optionalRef.get());
    }

    if (fieldInfo->kind == ReflectTypeKind::Class)
    {
        // Handle submessage
        auto lastMaxPosition = reader->maxPosition;
        auto messageSize = reader->decodeVarInt<uint32_t>();

        reader->maxPosition = messageSize + reader->pos;
        while (reader->next())
        {
            auto res = findFieldWithProtobufTag(any, reader->currentTag);
            if (res.has_value())
            {
                decodeField(reader, res.value());
            }
            else
            {
                reader->skip();
            }
        }
        reader->maxPosition = lastMaxPosition;
        return;
    } else if (any.is<std::vector<uint8_t>>())
    {
        reader->decodeVector(*any.as<std::vector<uint8_t>>());
    }
    // Handle repeated
    else if (fieldInfo->kind == ReflectTypeKind::Vector)
    {
        auto aVec = AnyVectorRef(any);
        aVec.emplace_back();
        auto value = aVec.at(aVec.size() - 1);
        auto valInfo = value.reflectType();

        // Represents packed repeated encoding
        if (valInfo->kind == ReflectTypeKind::Primitive && !value.is<std::string>() && !value.is<std::vector<uint8_t>>())
        {
            // *any.as<int64_t>() = reader->decodeVarInt<int64_t>();
            reader->skip();
        }
        else
        {
            decodeField(reader, value);
        }
    }
    else if (fieldInfo->kind == ReflectTypeKind::Enum)
    {
        *any.as<uint32_t>() = reader->decodeVarInt<uint32_t>();
    }
    else if (any.is<std::string>())
    {
        reader->decodeString(*any.as<std::string>());
    }
    else if (any.is<bool>())
    {
        *any.as<bool>() = reader->decodeVarInt<bool>();
    }
    else if (any.is<uint32_t>())
    {
        *any.as<uint32_t>() = reader->decodeVarInt<uint32_t>();
    }
    else if (any.is<int64_t>())
    {
        *any.as<int64_t>() = reader->decodeVarInt<int64_t>();
    }
    else
    {
        reader->skip();
    }
}

void decodeProtobuf(std::shared_ptr<PbReader> reader, AnyRef any)
{
    while (reader->next())
    {
        auto res = findFieldWithProtobufTag(any, reader->currentTag);
        if (res.has_value())
        {
            decodeField(reader, res.value());
        }
        else
        {
            reader->skip();
        }
    }
}

void encodeProtobuf(std::shared_ptr<PbWriter> writer, AnyRef any, uint32_t protobufTag)
{
    auto info = any.reflectType();

    // Handle optionals, only encode if have value
    if (info->kind == ReflectTypeKind::Optional)
    {
        auto optionalRef = AnyOptionalRef(any);
        if (!optionalRef.has_value())
        {
            return;
        }
        else
        {
            return encodeProtobuf(writer, optionalRef.get(), protobufTag);
        }
    }
    if (info->kind == ReflectTypeKind::Class)
    {
        uint32_t startMsgPosition;
        // 0 - default value, indicating top level message
        if (protobufTag > 0)
        {
            startMsgPosition = writer->startMessage();
        }

        for (int i = 0; i < info->fields.size(); i++)
        {
            auto field = any.getField(i);
            encodeProtobuf(writer, field, info->fields[i].protobufTag);
        }

        if (protobufTag > 0)
        {
            writer->finishMessage(protobufTag, startMsgPosition);
        }
    } else if (any.is<std::vector<uint8_t>>())
    {
        writer->addVector(protobufTag, *any.as<std::vector<uint8_t>>());
    }
    else if (info->kind == ReflectTypeKind::Vector) {
        auto aVec = AnyVectorRef(any);
        auto size = aVec.size();
        for (size_t i = 0; i < size; i++)
        {
            auto valueAt = aVec.at(i);
            encodeProtobuf(writer, valueAt, protobufTag);
        }
    }
    else if (info->kind == ReflectTypeKind::Enum) {
        writer->addVarInt<uint32_t>(protobufTag, *any.as<uint32_t>());
    }
    else if (info->kind == ReflectTypeKind::Primitive)
    {
        if (any.is<std::string>())
        {
            writer->addString(protobufTag, *any.as<std::string>());
        }
        else if (any.is<uint32_t>())
        {
            writer->addVarInt<uint32_t>(protobufTag, *any.as<uint32_t>());
        }
        else if (any.is<uint64_t>())
        {
            writer->addVarInt<uint64_t>(protobufTag, *any.as<uint64_t>());
        }
        else if (any.is<int64_t>()) {
            writer->addVarInt<int64_t>(protobufTag, *any.as<int64_t>());
        } else if (any.is<bool>())
        {
            writer->addVarInt<bool>(protobufTag, *any.as<bool>());
        }
    }
}