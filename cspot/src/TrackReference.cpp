#include "TrackReference.h"

TrackReference::TrackReference(TrackRef *ref)
{
    gid = ref->gid.value();
    // this->ref = ref;
    // if (ref->uri != nullptr)
    // {
    //     auto uri = ref->uri;
    //     auto idString = uri.substr(uri.find_last_of(":") + 1, uri.size());
    //     std::cout << idString << std::endl;
    //     gid = base62Decode(idString);
    //     isEpisode = true;
    // }
    // else
    // {
    //     gid = std::vector<uint8_t>(ref->gid->bytes, ref->gid->bytes + ref->gid->size);
    // }
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
