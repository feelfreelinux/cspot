#include "MercuryResponse.h"

MercuryResponse::MercuryResponse(std::vector<uint8_t> &data) {
    // this->mercuryHeader = std::make_unique<Header>();
    this->parts = mercuryParts(0);
    this->parseResponse(data);
}

void MercuryResponse::parseResponse(std::vector<uint8_t> &data) {
    this->sequenceId = htole64(extract<uint64_t>(data, 2));
    auto headerSize = ntohl(extract<uint32_t>(data, 13));
    auto headerBytes 
        = std::vector<uint8_t>(data.begin() + 15, data.begin() + 15 + headerSize);
    
    auto pos = 15 + headerSize;
    while (pos < data.size()) {
        auto partSize = ntohl(extract<uint32_t>(data, pos));
        this->parts.push_back(
            std::vector<uint8_t>(
                data.begin() + pos + 2, 
                data.begin() + pos + 2 + partSize));
        pos += 2 + partSize;
    }
    this->mercuryHeader = decodePB<Header>(Header_fields, headerBytes);
}