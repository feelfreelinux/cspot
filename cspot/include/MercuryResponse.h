
#ifndef MERCURYRESPONSE_H
#define MERCURYRESPONSE_H

#include <map>
#include <string>
#include <functional>
#include <vector>
#include <NanoPBHelper.h>
#include "protobuf/mercury.pb.h"
#include "Utils.h"

typedef std::vector<std::vector<uint8_t>> mercuryParts;

class MercuryResponse
{
private:
    void parseResponse(std::vector<uint8_t> &data);
    std::vector<uint8_t> data;
public:
    MercuryResponse(std::vector<uint8_t> &data);
    ~MercuryResponse();
    void decodeHeader();
    Header mercuryHeader;
    uint8_t flags;
    mercuryParts parts;
    uint64_t sequenceId;
};

#endif
