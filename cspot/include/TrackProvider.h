#pragma once

#include <stdint.h>  // for uint8_t
#include <memory>    // for shared_ptr, unique_ptr, weak_ptr
#include <vector>    // for vector

#include "MercurySession.h"        // for MercurySession
#include "TrackReference.h"        // for TrackReference
#include "protobuf/metadata.pb.h"  // for Episode, Restriction, Track

namespace cspot {
class AccessKeyFetcher;
class CDNTrackStream;
struct Context;

class TrackProvider {
 public:
  TrackProvider(std::shared_ptr<cspot::Context> ctx);
  ~TrackProvider();

  std::shared_ptr<CDNTrackStream> loadFromTrackRef(TrackReference& trackRef);

 private:
  std::shared_ptr<AccessKeyFetcher> accessKeyFetcher;
  std::shared_ptr<cspot::Context> ctx;
  std::unique_ptr<cspot::CDNTrackStream> cdnStream;

  Track trackInfo;
  Episode episodeInfo;
  std::weak_ptr<CDNTrackStream> currentTrackReference;
  TrackReference trackIdInfo;

  void queryMetadata();
  void onMetadataResponse(MercurySession::Response& res);
  bool doRestrictionsApply(Restriction* restrictions, int count);
  void fetchFile(const std::vector<uint8_t>& fileId,
                 const std::vector<uint8_t>& trackId);
  bool canPlayTrack(int index);
};
}  // namespace cspot