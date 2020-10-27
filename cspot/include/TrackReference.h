#ifndef TRACKREFERENCE_H
#define TRACKREFERENCE_H

#include <vector>
#include "spirc.pb.h"

class TrackReference {
private:

public:
    TrackReference(TrackRef* ref);
    std::vector<uint8_t> gid;
};

#endif