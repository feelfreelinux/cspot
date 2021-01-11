
#ifndef MERCURYRESPONSE_H
#define MERCURYRESPONSE_H

#include <map>
#include <string>
#include <functional>
#include <vector>
#include "ProtoHelper.h"
#include "Utils.h"

typedef std::vector<std::vector<uint8_t>> mercuryParts;

class MercuryResponse
{
private:
    void parseResponse(std::vector<uint8_t> &data);
    std::vector<uint8_t> data;
public:
    MercuryResponse(std::vector<uint8_t> &data);
    void decodeHeader();
    Header mercuryHeader;
    uint8_t flags;
    mercuryParts parts;
    uint64_t sequenceId;
};

#endif
