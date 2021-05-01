#include "TrackReference.h"
#include "Logger.h"

TrackReference::TrackReference(TrackRef *ref)
{
    if (ref->gid.has_value())
    {
        gid = ref->gid.value();
    }
    else if (ref->uri.has_value())
    {
        auto uri = ref->uri.value();
        auto idString = uri.substr(uri.find_last_of(":") + 1, uri.size());
        CSPOT_LOG(debug, "idString = %s", idString.c_str());
        gid = base62Decode(idString);
        isEpisode = true;
    }
}

TrackReference::~TrackReference()
{
}

std::vector<uint8_t> TrackReference::base62Decode(std::string uri)
{
    std::vector<uint8_t> n = std::vector<uint8_t>({0});

    for (int x = 0; x < uri.size(); x++)
    {
        size_t d = alphabet.find(uri[x]);
        n = bigNumMultiply(n, 62);
        n = bigNumAdd(n, d);
    }

    return n;
}
