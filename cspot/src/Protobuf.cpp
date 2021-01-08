#include "Protobuf.h"


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

// int main(int argc, char **argv)
// {
//     auto groBeKatze = std::vector<uint8_t>({82, 11, 98, 97, 114, 116, 101, 107, 112, 97, 99, 105, 97, 160, 1, 2, 242, 1, 4, 2, 1, 3, 7});
//     LoginCredentials kitku;
//     kitku.username="bartekpacia";
//     kitku.auth_data = std::vector<uint8_t>({2, 1, 3, 7});
//     Frame dupsko;
//     auto ref = AnyRef::of(&kitku);
//     std::vector<uint8_t> data = std::vector<uint8_t>({8, 1, 18, 40, 102, 98, 100, 56, 49, 100, 56, 55, 53, 100, 52, 56, 97, 48, 50, 98, 100, 57, 97, 56, 100, 52, 52, 97, 48, 56, 98, 54, 98, 51, 101, 52, 53, 97, 99, 48, 101, 48, 56, 57, 26, 5, 50, 46, 48, 46, 48, 32, 244, 189, 208, 48, 40, 10, 58, 165, 2, 10, 38, 101, 115, 100, 107, 58, 51, 46, 49, 50, 50, 46, 54, 55, 45, 103, 57, 99, 100, 102, 55, 100, 48, 101, 47, 116, 114, 97, 99, 107, 45, 112, 108, 97, 121, 98, 97, 99, 107, 80, 0, 88, 1, 96, 154, 51, 106, 11, 75, 68, 45, 53, 53, 88, 71, 56, 48, 57, 54, 138, 1, 4, 8, 2, 16, 1, 138, 1, 4, 8, 3, 16, 0, 138, 1, 4, 8, 5, 16, 1, 138, 1, 4, 8, 6, 16, 1, 138, 1, 4, 8, 7, 16, 1, 138, 1, 4, 8, 10, 16, 1, 138, 1, 4, 8, 11, 16, 0, 138, 1, 4, 8, 12, 16, 0, 138, 1, 4, 8, 4, 16, 5, 138, 1, 4, 8, 8, 16, 32, 138, 1, 4, 8, 13, 16, 0, 138, 1, 4, 8, 14, 16, 1, 138, 1, 30, 8, 9, 26, 11, 97, 117, 100, 105, 111, 47, 116, 114, 97, 99, 107, 26, 13, 97, 117, 100, 105, 111, 47, 101, 112, 105, 115, 111, 100, 101, 202, 1, 45, 10, 9, 99, 108, 105, 101, 110, 116, 95, 105, 100, 18, 32, 55, 53, 54, 97, 53, 50, 50, 100, 57, 102, 49, 54, 52, 56, 98, 56, 57, 101, 55, 54, 101, 56, 48, 98, 101, 54, 53, 52, 52, 53, 54, 97, 202, 1, 26, 10, 18, 98, 114, 97, 110, 100, 95, 100, 105, 115, 112, 108, 97, 121, 95, 110, 97, 109, 101, 18, 4, 83, 111, 110, 121, 202, 1, 36, 10, 18, 109, 111, 100, 101, 108, 95, 100, 105, 115, 112, 108, 97, 121, 95, 110, 97, 109, 101, 18, 14, 66, 82, 65, 86, 73, 65, 52, 75, 71, 66, 65, 84, 86, 51, 136, 1, 135, 184, 208, 176, 232, 46});
//     auto reader = std::make_shared<PbReader>(groBeKatze);
//     decodeProtobuf(reader, ref);
//     // printf("%d\n", dupsko.device_state->capabilities.size());
//     printf("OK\n");
//     std::vector<uint8_t> rawData;

//     auto writer = std::make_shared<PbWriter>(rawData);
//     encodeProtobuf(writer, ref);

//     for (int x = 0; x < rawData.size(); x++) {
//         printf("%d, ", rawData[x]);
//     }
//     printf("\n");
//     return 0;
// }