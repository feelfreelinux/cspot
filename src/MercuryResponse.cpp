#include "MercuryResponse.h"

MercuryResponse::MercuryResponse(std::vector<uint8_t> &data) {
    // this->mercuryHeader = std::make_unique<Header>();
    this->parts = mercuryParts(0);
    this->parseResponse(data);
}

void MercuryResponse::parseResponse(std::vector<uint8_t> &data) {
    auto sequenceLength = ntohs(extract<uint16_t>(data, 0));
    this->sequenceId = ntohl(extract<uint32_t>(data, 2));

    auto partsNumber = ntohs(extract<uint16_t>(data, 7));

    auto headerSize = ntohs(extract<uint16_t>(data, 9));
    auto headerBytes 
        = std::vector<uint8_t>(data.begin() + 11, data.begin() + 11 + headerSize);
    
    auto pos = 11 + headerSize;
    while (pos < data.size()) {
        auto partSize = ntohs(extract<uint16_t>(data, pos));

        this->parts.push_back(
            std::vector<uint8_t>(
                data.begin() + pos + 2, 
                data.begin() + pos + 2 + partSize));
        pos += 2 + partSize;
    }

    this->mercuryHeader = decodePB<Header>(Header_fields, headerBytes);
}