#include "TrackReference.h"

#include "NanoPBExtensions.h"
#include "protobuf/connect.pb.h"

using namespace cspot;

static std::string empty_string = "";

TrackReference::TrackReference() : type(Type::TRACK) {}

void TrackReference::decodeURI() {
  if (gid.size() == 0) {
    // Episode GID is being fetched via base62 encoded URI
    gid = base62Decode(uri).second;

    if (uri.find("episode:") != std::string::npos) {
      type = Type::EPISODE;
    }
  }
}

bool TrackReference::operator==(const TrackReference& other) const {
  return other.gid == gid && other.uri == uri;
}

bool TrackReference::pbEncodeProvidedTracks(pb_ostream_t* stream,
                                            const pb_field_t* field,
                                            void* const* arg) {
  auto trackPacket = (std::pair<uint8_t*, std::vector<ProvidedTrack>*>*)(*arg);
  if (*trackPacket->first >= trackPacket->second->size() ||
      !trackPacket->second->size())
    return true;
  for (int i = *trackPacket->first; i < trackPacket->second->size(); i++) {
    if (!pb_encode_tag_for_field(stream, field)) {
      return false;
    }
    if (!pb_encode_submessage(stream, ProvidedTrack_fields,
                              &(trackPacket->second->at(i))))
      return false;
    //if there's a delimiter, or the tracks are over the track treshhold
    if (trackPacket->second->at(i).removed != NULL ||
        i - *trackPacket->first >= TRACK_SEND_LIMIT)
      return true;
  }
  return true;
}

bool TrackReference::pbDecodeProvidedTracks(pb_istream_t* stream,
                                            const pb_field_t* field,
                                            void** arg) {
  auto trackQueue = static_cast<std::vector<ProvidedTrack>*>(*arg);

  // Push a new reference
  trackQueue->push_back(ProvidedTrack());

  auto& track = trackQueue->back();

  if (!pb_decode(stream, ProvidedTrack_fields, &track))
    return false;
  track.full_metadata_count = track.metadata_count;

  return true;
}