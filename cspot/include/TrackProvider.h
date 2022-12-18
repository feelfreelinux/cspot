#pragma once

#include <memory>

#include "AccessKeyFetcher.h"
#include "CDNTrackStream.h"
#include "CSpotContext.h"
#include "protobuf/metadata.pb.h"
#include "protobuf/spirc.pb.h"

namespace cspot {
class TrackProvider {
 public:
  TrackProvider(std::shared_ptr<cspot::Context> ctx);
  ~TrackProvider();

  std::shared_ptr<CDNTrackStream> loadFromTrackRef(TrackRef* trackRef);

 private:
  std::shared_ptr<AccessKeyFetcher> accessKeyFetcher;
  std::shared_ptr<cspot::Context> ctx;
  std::unique_ptr<cspot::CDNTrackStream> cdnStream;

  // For BASE62 decoding
  std::string alphabet =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::vector<uint8_t> gid;

  enum class Type { TRACK, EPISODE };
  Type trackType;
  Track trackInfo;
  std::weak_ptr<CDNTrackStream> currentTrackReference;

  void queryMetadata();
  void onMetadataResponse(MercurySession::Response& res);
  void fetchFile(const std::vector<uint8_t>& fileId,
                 const std::vector<uint8_t>& trackId);
  bool canPlayTrack(int index);
};
}  // namespace cspot