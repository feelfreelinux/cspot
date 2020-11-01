#include "TimeProvider.h"
#include "Utils.h"

TimeProvider::TimeProvider() {
}

void TimeProvider::syncWithPingPacket(const std::vector<uint8_t>& pongPacket) {
    // Spotify's timestamp is in seconds since unix time - convert to millis.
    uint64_t remoteTimestamp =  ((uint64_t) ntohl(extract<uint32_t>(pongPacket, 0))) * 1000;
    this->timestampDiff = remoteTimestamp - getCurrentTimestamp();
}

unsigned long long TimeProvider::getSyncedTimestamp() {
    return getCurrentTimestamp() + this->timestampDiff;
}