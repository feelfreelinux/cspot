#include "TrackProvider.h"

#include <assert.h>                // for assert
#include <string.h>                // for strlen
#include <cstdint>                 // for uint8_t
#include <functional>              // for __base
#include <memory>                  // for shared_ptr, weak_ptr, make_shared
#include <string>                  // for string, operator+
#include <type_traits>             // for remove_extent_t

#include "AccessKeyFetcher.h"      // for AccessKeyFetcher
#include "BellLogger.h"            // for AbstractLogger
#include "CDNTrackStream.h"        // for CDNTrackStream, CDNTrackStream::Tr...
#include "CSpotContext.h"          // for Context::ConfigState, Context (ptr...
#include "Logger.h"                // for CSPOT_LOG
#include "MercurySession.h"        // for MercurySession, MercurySession::Da...
#include "NanoPBHelper.h"          // for pbArrayToVector, pbDecode
#include "Packet.h"                // for cspot
#include "TrackReference.h"        // for TrackReference, TrackReference::Type
#include "Utils.h"                 // for bytesToHexString, string_format
#include "WrappedSemaphore.h"      // for WrappedSemaphore
#include "pb_decode.h"             // for pb_release
#include "protobuf/metadata.pb.h"  // for Track, _Track, AudioFile, Episode

using namespace cspot;

TrackProvider::TrackProvider(std::shared_ptr<cspot::Context> ctx) {
  this->accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);
  this->ctx = ctx;
  this->cdnStream =
      std::make_unique<cspot::CDNTrackStream>(this->accessKeyFetcher);

  this->trackInfo = {};
}

TrackProvider::~TrackProvider() {
  pb_release(Track_fields, &trackInfo);
  pb_release(Episode_fields, &trackInfo);
}

std::shared_ptr<cspot::CDNTrackStream> TrackProvider::loadFromTrackRef(
    TrackReference& trackRef) {
  auto track = std::make_shared<cspot::CDNTrackStream>(this->accessKeyFetcher);
  this->currentTrackReference = track;
  this->trackIdInfo = trackRef;

  queryMetadata();
  return track;
}

void TrackProvider::queryMetadata() {
  std::string requestUrl = string_format(
      "hm://metadata/3/%s/%s",
      trackIdInfo.type == TrackReference::Type::TRACK ? "track" : "episode",
      bytesToHexString(trackIdInfo.gid).c_str());
  CSPOT_LOG(debug, "Requesting track metadata from %s", requestUrl.c_str());

  auto responseHandler = [this](MercurySession::Response& res) {
    this->onMetadataResponse(res);
  };

  // Execute the request
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}

void TrackProvider::onMetadataResponse(MercurySession::Response& res) {
  CSPOT_LOG(debug, "Got track metadata response");

  int alternativeCount, filesCount = 0;
  bool canPlay = false;
  AudioFile* selectedFiles;
  std::vector<uint8_t> trackId, fileId;

  if (trackIdInfo.type == TrackReference::Type::TRACK) {
    pb_release(Track_fields, &trackInfo);
    assert(res.parts.size() > 0);
    pbDecode(trackInfo, Track_fields, res.parts[0]);
    CSPOT_LOG(info, "Track name: %s", trackInfo.name);
    CSPOT_LOG(info, "Track duration: %d", trackInfo.duration);

    CSPOT_LOG(debug, "trackInfo.restriction.size() = %d",
              trackInfo.restriction_count);

    if (doRestrictionsApply(trackInfo.restriction,
                            trackInfo.restriction_count)) {
      // Go through alternatives
      for (int x = 0; x < trackInfo.alternative_count; x++) {
        if (!doRestrictionsApply(trackInfo.alternative[x].restriction,
                                 trackInfo.alternative[x].restriction_count)) {
          selectedFiles = trackInfo.alternative[x].file;
          filesCount = trackInfo.alternative[x].file_count;
          trackId = pbArrayToVector(trackInfo.alternative[x].gid);
          break;
        }
      }
    } else {
      selectedFiles = trackInfo.file;
      filesCount = trackInfo.file_count;
      trackId = pbArrayToVector(trackInfo.gid);
    }

    // Set track's metadata
    auto trackRef = this->currentTrackReference.lock();

    auto imageId =
        pbArrayToVector(trackInfo.album.cover_group.image[0].file_id);

    trackRef->trackInfo.trackId = bytesToHexString(trackIdInfo.gid);
    trackRef->trackInfo.name = std::string(trackInfo.name);
    trackRef->trackInfo.album = std::string(trackInfo.album.name);
    trackRef->trackInfo.artist = std::string(trackInfo.artist[0].name);
    trackRef->trackInfo.imageUrl =
        "https://i.scdn.co/image/" + bytesToHexString(imageId);
    trackRef->trackInfo.duration = trackInfo.duration;
  } else {
    pb_release(Episode_fields, &episodeInfo);
    assert(res.parts.size() > 0);
    pbDecode(episodeInfo, Episode_fields, res.parts[0]);

    CSPOT_LOG(info, "Episode name: %s", episodeInfo.name);
    CSPOT_LOG(info, "Episode duration: %d", episodeInfo.duration);

    CSPOT_LOG(debug, "episodeInfo.restriction.size() = %d",
              episodeInfo.restriction_count);
    if (!doRestrictionsApply(episodeInfo.restriction,
                             episodeInfo.restriction_count)) {
      selectedFiles = episodeInfo.file;
      filesCount = episodeInfo.file_count;
      trackId = pbArrayToVector(episodeInfo.gid);
    }

    auto trackRef = this->currentTrackReference.lock();

    auto imageId = pbArrayToVector(episodeInfo.covers->image[0].file_id);

    trackRef->trackInfo.trackId = bytesToHexString(trackIdInfo.gid);
    trackRef->trackInfo.name = std::string(episodeInfo.name);
    trackRef->trackInfo.album = "";
    trackRef->trackInfo.artist = "",
    trackRef->trackInfo.imageUrl =
        "https://i.scdn.co/image/" + bytesToHexString(imageId);
    trackRef->trackInfo.duration = episodeInfo.duration;
  }

  for (int x = 0; x < filesCount; x++) {
    CSPOT_LOG(debug, "File format: %d", selectedFiles[x].format);
    if (selectedFiles[x].format == ctx->config.audioFormat) {
      fileId = pbArrayToVector(selectedFiles[x].file_id);
      break;  // If file found stop searching
    }

    // Fallback to OGG Vorbis 96kbps
    if (fileId.size() == 0 &&
        selectedFiles[x].format == AudioFormat_OGG_VORBIS_96) {
      fileId = pbArrayToVector(selectedFiles[x].file_id);
    }
  }

  // No viable files found for playback
  if (fileId.size() == 0) {
    CSPOT_LOG(info, "File not available for playback");
    // no alternatives for song
    if (!this->currentTrackReference.expired()) {
      auto trackRef = this->currentTrackReference.lock();
      trackRef->status = CDNTrackStream::Status::FAILED;
      trackRef->trackReady->give();
    }
    return;
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

bool countryListContains(char* countryList, char* country) {
  uint16_t countryList_length = strlen(countryList);
  for (int x = 0; x < countryList_length; x += 2) {
    if (countryList[x] == country[0] && countryList[x + 1] == country[1]) {
      return true;
    }
  }
  return false;
}

bool TrackProvider::doRestrictionsApply(Restriction* restrictions, int count) {
  for (int x = 0; x < count; x++) {
    if (restrictions[x].countries_allowed != nullptr) {
      return !countryListContains(restrictions[x].countries_allowed,
                                  (char*)ctx->config.countryCode.c_str());
    }

    if (restrictions[x].countries_forbidden != nullptr) {
      return countryListContains(restrictions[x].countries_forbidden,
                                 (char*)ctx->config.countryCode.c_str());
    }
  }

  return false;
}

bool TrackProvider::canPlayTrack(int altIndex) {
  if (altIndex < 0) {

  } else {
    for (int x = 0; x < trackInfo.alternative[altIndex].restriction_count;
         x++) {
      if (trackInfo.alternative[altIndex].restriction[x].countries_allowed !=
          nullptr) {
        return countryListContains(
            trackInfo.alternative[altIndex].restriction[x].countries_allowed,
            (char*)ctx->config.countryCode.c_str());
      }

      if (trackInfo.alternative[altIndex].restriction[x].countries_forbidden !=
          nullptr) {
        return !countryListContains(
            trackInfo.alternative[altIndex].restriction[x].countries_forbidden,
            (char*)ctx->config.countryCode.c_str());
      }
    }
  }
  return true;
}
