#include "TrackProvider.h"
#include <memory>
#include "AccessKeyFetcher.h"
#include "CDNTrackStream.h"
#include "Logger.h"
#include "MercurySession.h"
#include "Utils.h"
#include "protobuf/metadata.pb.h"

using namespace cspot;

TrackProvider::TrackProvider(std::shared_ptr<cspot::Context> ctx) {
  this->accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);
  this->ctx = ctx;
  this->cdnStream = std::make_unique<cspot::CDNTrackStream>(ctx);
}

TrackProvider::~TrackProvider() {}

std::shared_ptr<cspot::CDNTrackStream> TrackProvider::loadFromTrackRef(
    TrackRef* ref) {
  auto track = std::make_shared<cspot::CDNTrackStream>(ctx);
  this->currentTrackReference = track;

  if (ref->gid != nullptr) {
    // For tracks, the GID is already in the protobuf
    gid = pbArrayToVector(ref->gid);
    trackType = Type::TRACK;
  } else if (ref->uri != nullptr) {
    // Episode GID is being fetched via base62 encoded URI
    auto uri = std::string(ref->uri);
    auto idString = uri.substr(uri.find_last_of(":") + 1, uri.size());

    gid = {0};

    for (int x = 0; x < uri.size(); x++) {
      size_t d = alphabet.find(uri[x]);
      gid = bigNumMultiply(gid, 62);
      gid = bigNumAdd(gid, d);
    }

    trackType = Type::EPISODE;
  }

  queryMetadata();

  return track;
}

void TrackProvider::queryMetadata() {
  std::string requestUrl = string_format(
      "hm://metadata/3/%s/%s", trackType == Type::TRACK ? "track" : "episode",
      bytesToHexString(gid).c_str());
  CSPOT_LOG(debug, "Requesting track metadata from %s", requestUrl.c_str());

  auto responseHandler = [=](MercurySession::Response& res) {
    this->onMetadataResponse(res);
  };

  // Execute the request
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}

void TrackProvider::onMetadataResponse(MercurySession::Response& res) {
  CSPOT_LOG(debug, "Got track metadata response");

  pb_release(Track_fields, &trackInfo);
  pbDecode(trackInfo, Track_fields, res.parts[0]);

  CSPOT_LOG(info, "Track name: %s", trackInfo.name);
  CSPOT_LOG(info, "Track duration: %d", trackInfo.duration);
  CSPOT_LOG(debug, "trackInfo.restriction.size() = %d",
            trackInfo.restriction_count);

  int altIndex = -1;
  while (!canPlayTrack(altIndex)) {
    altIndex++;
    CSPOT_LOG(info, "Trying alternative %d", altIndex);

    if (altIndex >= trackInfo.alternative_count) {
      // no alternatives for song
      return;
    }
  }

  std::vector<uint8_t> trackId;
  std::vector<uint8_t> fileId;
  AudioFormat format = AudioFormat_OGG_VORBIS_160;

  if (altIndex < 0) {
    trackId = pbArrayToVector(trackInfo.gid);
    for (int x = 0; x < trackInfo.file_count; x++) {
      if (trackInfo.file[x].format == format) {
        fileId = pbArrayToVector(trackInfo.file[x].file_id);
        break;  // If file found stop searching
      }
    }
  } else {
    trackId = pbArrayToVector(trackInfo.alternative[altIndex].gid);
    for (int x = 0; x < trackInfo.alternative[altIndex].file_count; x++) {
      if (trackInfo.alternative[altIndex].file[x].format == format) {
        fileId =
            pbArrayToVector(trackInfo.alternative[altIndex].file[x].file_id);
        break;  // If file found stop searching
      }
    }
  }

  this->fetchFile(fileId, trackId);
}

void TrackProvider::fetchFile(const std::vector<uint8_t>& fileId,
                              const std::vector<uint8_t>& trackId) {
  ctx->session->requestAudioKey(
      trackId, fileId,
      [this, fileId](bool success, const std::vector<uint8_t>& audioKey) {
        if (success) {
          CSPOT_LOG(info, "Got audio key");
          if (!this->currentTrackReference.expired()) {
            auto ref = this->currentTrackReference.lock();
            ref->fetchFile(fileId, audioKey);
          }

        } else {
          CSPOT_LOG(error, "Failed to get audio key");
          if (!this->currentTrackReference.expired()) {
            auto ref = this->currentTrackReference.lock();
            ref->fail();
          }
        }
      });
}

bool TrackProvider::canPlayTrack(int altIndex) {
  return true;
}