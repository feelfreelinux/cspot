#pragma once

#include <string_view>
#include <vector>
#include "NanoPBHelper.h"
#include "Utils.h"
#include "protobuf/spirc.pb.h"
namespace cspot {

struct TrackReference {
  static constexpr auto base62Alphabet =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  // Resolved track GID
  std::vector<uint8_t> gid;

  // Type identifier
  enum class Type { TRACK, EPISODE };
  Type type;

  static TrackReference fromTrackRef(TrackRef* ref) {
    TrackReference trackRef;
    if (ref->gid != nullptr) {
      // For tracks, the GID is already in the protobuf
      trackRef.gid = pbArrayToVector(ref->gid);
      trackRef.type = Type::TRACK;
    } else {
      // Episode GID is being fetched via base62 encoded URI
      auto uri = std::string(ref->uri);
      auto idString = uri.substr(uri.find_last_of(":") + 1, uri.size());
      trackRef.gid = {0};

      std::string_view alphabet(base62Alphabet);
      for (int x = 0; x < idString.size(); x++) {
        size_t d = alphabet.find(idString[x]);
        trackRef.gid = bigNumMultiply(trackRef.gid, 62);
        trackRef.gid = bigNumAdd(trackRef.gid, d);
      }
    }

    return trackRef;
  }

  static TrackReference fromGID(std::vector<uint8_t> gid, bool isEpisode) {
    TrackReference trackRef;
    trackRef.gid = gid;
    trackRef.type = isEpisode ? Type::EPISODE : Type::TRACK;
    return trackRef;
  }
};
}  // namespace cspot