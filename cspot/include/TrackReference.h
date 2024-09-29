#pragma once

#include <pb_encode.h>
#include <optional>
#include <string_view>
#include <vector>
#include "NanoPBHelper.h"
#include "Utils.h"  //for base62decode
#include "pb_decode.h"
#include "protobuf/connect.pb.h"

#define TRACK_SEND_LIMIT 25

namespace cspot {
struct TrackReference {
  TrackReference();
  TrackReference(std::string uri, std::string context) : type(Type::TRACK) {
    this->gid = base62Decode(uri).second;
    //this->uri=uri;
    this->context = context;
  }
  TrackReference(std::string uri) : type(Type::TRACK) {
    gid = base62Decode(uri).second;

    if (uri.find("episode:") != std::string::npos) {
      type = Type::EPISODE;
    }
    this->uri = uri;
  }

  // Resolved track GID
  std::vector<uint8_t> gid;
  std::string uri, context;
  std::optional<bool> queued;

  // Type identifier
  enum class Type { TRACK, EPISODE };

  Type type;

  void decodeURI();

  bool operator==(const TrackReference& other) const;

  // Encodes list of track references into a pb structure, used by nanopb
  static bool pbEncodeProvidedTracks(pb_ostream_t* stream,
                                     const pb_field_t* field, void* const* arg);

  static bool pbDecodeProvidedTracks(pb_istream_t* stream,
                                     const pb_field_t* field, void** arg);

  static void clearProvidedTracklist(std::vector<ProvidedTrack>* tracklist) {
    for (auto& track : *tracklist)
      pbReleaseProvidedTrack(&track);
    tracklist->clear();
  }

  static void pbReleaseProvidedTrack(ProvidedTrack* track) {
    if (track->metadata_count < track->full_metadata_count)
      track->metadata_count = track->full_metadata_count;
    pb_release(ProvidedTrack_fields, track);
  }
};
}  // namespace cspot
