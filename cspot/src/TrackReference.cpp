#include "TrackReference.h"

TrackReference::TrackReference(TrackRef* ref) {
    gid = std::vector<uint8_t>(ref->gid->bytes, ref->gid->bytes + 16);
}