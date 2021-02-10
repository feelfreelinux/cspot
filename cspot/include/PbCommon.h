#ifndef PBCOMMON_H
#define PBCOMMON_H

#include <cstdint>
#include <PbWriter.h>
#include <optional>
#include <PbReader.h>
class BaseProtobufMessage
{
private:
public:
    bool firstField = true;
    uint32_t lastMessagePosition;
    void parseFromVector(std::vector<uint8_t> const &rawData)
    {
        auto reader = std::make_shared<PbReader>(rawData);
        parseWithReader(reader);
    }
    void encodeToVector(std::vector<uint8_t> &rawData)
    {
        auto writer = std::make_shared<PbWriter>(rawData);
        encodeWithWriter(writer);
    }

    void parseWithReader(std::shared_ptr<PbReader> reader)
    {
        firstField = true;
        while (reader->next())
        {
            if (!decodeField(reader))
            {
                reader->skip();
            } else {
                firstField = false;
            }
        }
    }
    virtual void encodeWithWriter(std::shared_ptr<PbWriter> writer) = 0;
    virtual bool decodeField(std::shared_ptr<PbReader> reader) = 0;
};

#endif