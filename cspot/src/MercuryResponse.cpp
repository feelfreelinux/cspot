#include "MercuryResponse.h"

MercuryResponse::MercuryResponse(std::vector<uint8_t> &data)
{
    // this->mercuryHeader = std::make_unique<Header>();
    this->mercuryHeader = {};
    this->parts = mercuryParts(0);
    this->parseResponse(data);
}

MercuryResponse::~MercuryResponse() {
}

void MercuryResponse::parseResponse(std::vector<uint8_t> &data)
{
    auto sequenceLength = ntohs(extract<uint16_t>(data, 0));
    this->sequenceId = hton64(extract<uint64_t>(data, 2));

    auto partsNumber = ntohs(extract<uint16_t>(data, 11));

    auto headerSize = ntohs(extract<uint16_t>(data, 13));
    auto headerBytes = std::vector<uint8_t>(data.begin() + 15, data.begin() + 15 + headerSize);

    auto pos = 15 + headerSize;
    while (pos < data.size())
    {
        auto partSize = ntohs(extract<uint16_t>(data, pos));

        this->parts.push_back(
            std::vector<uint8_t>(
                data.begin() + pos + 2,
                data.begin() + pos + 2 + partSize));
        pos += 2 + partSize;
    }

    pb_release(Header_fields, &this->mercuryHeader);
    pbDecode(this->mercuryHeader, Header_fields, headerBytes);
}