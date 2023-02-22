#pragma once

#include <memory>

#include "AccessKeyFetcher.h"
#include "CDNTrackStream.h"
#include "CSpotContext.h"
#include "TrackReference.h"
#include "protobuf/metadata.pb.h"
#include "protobuf/spirc.pb.h"

namespace cspot {
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
  std::weak_ptr<CDNTrackStream> currentTrackReference;
  TrackReference trackIdInfo;

  void queryMetadata();
  void onMetadataResponse(MercurySession::Response& res);
  void fetchFile(const std::vector<uint8_t>& fileId,
                 const std::vector<uint8_t>& trackId);
  bool canPlayTrack(int index);
};
}  // namespace cspot