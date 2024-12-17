#include "TrackQueue.h"
#include <pb_decode.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <random>

#include "AccessKeyFetcher.h"
#include "BellTask.h"
#include "BellUtils.h"  // for BELL_SLEEP_MS
#include "CDNAudioFile.h"
#include "CSpotContext.h"
#include "HTTPClient.h"
#include "Logger.h"
#include "Utils.h"
#include "WrappedSemaphore.h"
#ifdef BELL_ONLY_CJSON
#include "cJSON.h"
#else
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#endif
#include "protobuf/metadata.pb.h"

using namespace cspot;

namespace TrackDataUtils {
bool countryListContains(char* countryList, const char* country) {
  uint16_t countryList_length = strlen(countryList);
  for (int x = 0; x < countryList_length; x += 2) {
    if (countryList[x] == country[0] && countryList[x + 1] == country[1]) {
      return true;
    }
  }
  return false;
}

bool doRestrictionsApply(Restriction* restrictions, int count,
                         const char* country) {
  for (int x = 0; x < count; x++) {
    if (restrictions[x].countries_allowed != nullptr) {
      return !countryListContains(restrictions[x].countries_allowed, country);
    }

    if (restrictions[x].countries_forbidden != nullptr) {
      return countryListContains(restrictions[x].countries_forbidden, country);
    }
  }

  return false;
}

bool canPlayTrack(Track& trackInfo, int altIndex, const char* country) {
  if (altIndex < 0) {

  } else {
    for (int x = 0; x < trackInfo.alternative[altIndex].restriction_count;
         x++) {
      if (trackInfo.alternative[altIndex].restriction[x].countries_allowed !=
          nullptr) {
        return countryListContains(
            trackInfo.alternative[altIndex].restriction[x].countries_allowed,
            country);
      }

      if (trackInfo.alternative[altIndex].restriction[x].countries_forbidden !=
          nullptr) {
        return !countryListContains(
            trackInfo.alternative[altIndex].restriction[x].countries_forbidden,
            country);
      }
    }
  }
  return true;
}
}  // namespace TrackDataUtils

void TrackInfo::loadPbTrack(Track* pbTrack, const std::vector<uint8_t>& gid) {
  // Generate ID based on GID
  trackId = bytesToHexString(gid);

  name = std::string(pbTrack->name);

  if (pbTrack->artist_count > 0) {
    // Handle artist data
    artist = std::string(pbTrack->artist[0].name);
  }

  if (pbTrack->has_album) {
    // Handle album data
    album = std::string(pbTrack->album.name);

    if (pbTrack->album.has_cover_group &&
        pbTrack->album.cover_group.image_count > 0) {
      auto imageId =
          pbArrayToVector(pbTrack->album.cover_group.image[0].file_id);
      imageUrl = "https://i.scdn.co/image/" + bytesToHexString(imageId);
    }
  }

  number = pbTrack->has_number ? pbTrack->number : 0;
  discNumber = pbTrack->has_disc_number ? pbTrack->disc_number : 0;
  duration = pbTrack->duration;
}

void TrackInfo::loadPbEpisode(Episode* pbEpisode,
                              const std::vector<uint8_t>& gid) {
  // Generate ID based on GID
  trackId = bytesToHexString(gid);

  name = std::string(pbEpisode->name);

  if (pbEpisode->covers->image_count > 0) {
    // Handle episode info
    auto imageId = pbArrayToVector(pbEpisode->covers->image[0].file_id);
    imageUrl = "https://i.scdn.co/image/" + bytesToHexString(imageId);
  }

  number = pbEpisode->has_number ? pbEpisode->number : 0;
  discNumber = 0;
  duration = pbEpisode->duration;
}

QueuedTrack::QueuedTrack(
    ProvidedTrack& ref, std::shared_ptr<cspot::Context> ctx,
    std::shared_ptr<bell::WrappedSemaphore> playableSemaphore,
    int64_t requestedPosition)
    : requestedPosition((uint32_t)requestedPosition), ctx(ctx) {
  trackMetrics = std::make_shared<TrackMetrics>(ctx, requestedPosition);
  this->playableSemaphore = playableSemaphore;
  this->ref = ref;
  this->audioFormat = ctx->config.audioFormat;
  if (!strstr(ref.uri, "spotify:delimiter")) {
    this->gid = base62Decode(ref.uri);
    state = State::QUEUED;
  } else {
    state = State::FAILED;
    playableSemaphore->give();
  }
}

QueuedTrack::~QueuedTrack() {
  //if (state < State::READY)
  //  playableSemaphore->give();
  state = State::FAILED;

  if (pendingMercuryRequest != 0) {
    ctx->session->unregister(pendingMercuryRequest);
  }

  if (pendingAudioKeyRequest != 0) {
    ctx->session->unregisterAudioKey(pendingAudioKeyRequest);
  }
  pb_release(Track_fields, &pbTrack);
  pb_release(Episode_fields, &pbEpisode);
}

std::shared_ptr<cspot::CDNAudioFile> QueuedTrack::getAudioFile() {
  if (state != State::READY) {
    return nullptr;
  }

  return std::make_shared<cspot::CDNAudioFile>(cdnUrl, audioKey);
}

void QueuedTrack::stepParseMetadata(Track* pbTrack, Episode* pbEpisode) {
  int alternativeCount, filesCount = 0;
  bool canPlay = false;
  AudioFile* selectedFiles = nullptr;

  const char* countryCode = ctx->session->getCountryCode().c_str();

  if (gid.first == SpotifyFileType::TRACK) {
    CSPOT_LOG(info, "Track name: %s", pbTrack->name);
    CSPOT_LOG(info, "Track duration: %d", pbTrack->duration);

    // Check if we can play the track, if not, try alternatives
    if (TrackDataUtils::doRestrictionsApply(
            pbTrack->restriction, pbTrack->restriction_count, countryCode)) {
      // Go through alternatives
      for (int x = 0; x < pbTrack->alternative_count; x++) {
        if (!TrackDataUtils::doRestrictionsApply(
                pbTrack->alternative[x].restriction,
                pbTrack->alternative[x].restriction_count, countryCode)) {
          selectedFiles = pbTrack->alternative[x].file;
          filesCount = pbTrack->alternative[x].file_count;
          trackId = pbArrayToVector(pbTrack->alternative[x].gid);
          break;
        }
      }
    } else {
      // We can play the track
      selectedFiles = pbTrack->file;
      filesCount = pbTrack->file_count;
      trackId = pbArrayToVector(pbTrack->gid);
    }

    if (trackId.size() > 0) {
      // Load track information
      trackInfo.loadPbTrack(pbTrack, trackId);
    }
  } else {
    // Handle episodes
    CSPOT_LOG(info, "Episode name: %s", pbEpisode->name);
    CSPOT_LOG(info, "Episode duration: %d", pbEpisode->duration);

    // Check if we can play the episode
    if (!TrackDataUtils::doRestrictionsApply(pbEpisode->restriction,
                                             pbEpisode->restriction_count,
                                             countryCode)) {
      selectedFiles = pbEpisode->file;
      filesCount = pbEpisode->file_count;
      trackId = pbArrayToVector(pbEpisode->gid);

      // Load track information
      trackInfo.loadPbEpisode(pbEpisode, trackId);
    }
  }

  // Find playable file
  for (int x = 0; x < filesCount; x++) {
    CSPOT_LOG(debug, "File format: %d", selectedFiles[x].format);
    if (selectedFiles[x].format == audioFormat) {
      fileId = pbArrayToVector(selectedFiles[x].file_id);
      break;  // If file found stop searching
    }

    // Fallback to OGG Vorbis 96kbps
    if (fileId.size() == 0 &&
        selectedFiles[x].format == AudioFormat_OGG_VORBIS_96) {
      fileId = pbArrayToVector(selectedFiles[x].file_id);
      CSPOT_LOG(info, "Falling back to OGG Vorbis 96kbps");
    }
  }

  // No viable files found for playback
  if (fileId.size() == 0) {
    CSPOT_LOG(info, "File not available for playback");

    // no alternatives for song
    state = State::FAILED;
    playableSemaphore->give();
    return;
  }
  identifier = bytesToHexString(fileId);
  state = State::KEY_REQUIRED;
}

void QueuedTrack::stepLoadAudioFile(
    std::mutex& trackListMutex,
    std::shared_ptr<bell::WrappedSemaphore> updateSemaphore) {
  // Request audio key
  this->pendingAudioKeyRequest = ctx->session->requestAudioKey(
      trackId, fileId,
      [this, &trackListMutex, updateSemaphore](
          bool success, const std::vector<uint8_t>& audioKey) {
        std::scoped_lock lock(trackListMutex);

        if (success) {
          CSPOT_LOG(info, "Got audio key");
          this->audioKey =
              std::vector<uint8_t>(audioKey.begin() + 4, audioKey.end());

          state = State::CDN_REQUIRED;
          updateSemaphore->give();
        } else {
          CSPOT_LOG(error, "Failed to get audio key");
          retries++;
          state = State::KEY_REQUIRED;
          if (retries > 10) {
            if (audioFormat > AudioFormat_OGG_VORBIS_96) {
              audioFormat = (AudioFormat)(audioFormat - 1);
              state = State::QUEUED;
            } else {
              state = State::FAILED;
              playableSemaphore->give();
            }
          }
        }
      });

  state = State::PENDING_KEY;
}

void QueuedTrack::stepLoadCDNUrl(const std::string& accessKey) {
  if (accessKey.size() == 0) {
    // Wait for access key
    return;
  }

  // Request CDN URL
  CSPOT_LOG(info, "Received access key, fetching CDN URL...");

  try {

    std::string requestUrl = string_format(
        "https://api.spotify.com/v1/storage-resolve/files/audio/interactive/"
        "%s?alt=json",
        bytesToHexString(fileId).c_str());
    auto req = bell::HTTPClient::get(
        requestUrl, {bell::HTTPClient::ValueHeader(
                        {"Authorization", "Bearer " + accessKey})});

    // Wait for response
    std::string_view result = req->body();

#ifdef BELL_ONLY_CJSON
    cJSON* jsonResult = cJSON_Parse(result.data());
    cdnUrl = cJSON_GetArrayItem(cJSON_GetObjectItem(jsonResult, "cdnurl"), 0)
                 ->valuestring;
    cJSON_Delete(jsonResult);
#else
    auto jsonResult = nlohmann::json::parse(result);
    cdnUrl = jsonResult["cdnurl"][0];
#endif

    // CSPOT_LOG(info, "Received CDN URL, %s", cdnUrl.c_str());
    state = State::READY;
  } catch (...) {
    CSPOT_LOG(error, "Cannot fetch CDN URL");
    state = State::FAILED;
  }
  playableSemaphore->give();
}

void QueuedTrack::stepLoadMetadata(
    Track* pbTrack, Episode* pbEpisode, std::mutex& trackListMutex,
    std::shared_ptr<bell::WrappedSemaphore> updateSemaphore) {
  // Prepare request ID
  std::string requestUrl =
      string_format("hm://metadata/3/%s/%s",
                    gid.first == SpotifyFileType::TRACK ? "track" : "episode",
                    bytesToHexString(gid.second).c_str());

  auto responseHandler = [this, pbTrack, pbEpisode, &trackListMutex,
                          updateSemaphore](MercurySession::Response res) {
    std::scoped_lock lock(trackListMutex);

    if (res.parts.size() == 0) {
      CSPOT_LOG(info, "Invalid Metadata");
      // Invalid metadata, cannot proceed
      state = State::FAILED;
      updateSemaphore->give();
      playableSemaphore->give();
      return;
    }

    // Parse the metadata
    if (gid.first == SpotifyFileType::TRACK) {
      pb_release(Track_fields, pbTrack);
      pbDecode(*pbTrack, Track_fields, res.parts[0]);
    } else {
      pb_release(Episode_fields, pbEpisode);
      pbDecode(*pbEpisode, Episode_fields, res.parts[0]);
    }

    // Parse received metadata
    stepParseMetadata(pbTrack, pbEpisode);

    updateSemaphore->give();
  };
  // Execute the request
  if (pbTrack != NULL || pbEpisode != NULL)
    pendingMercuryRequest = ctx->session->execute(
        MercurySession::RequestType::GET, requestUrl, responseHandler);

  // Set the state to pending
  state = State::PENDING_META;
}

TrackQueue::TrackQueue(std::shared_ptr<cspot::Context> ctx)
    : bell::Task("CSpotTrackQueue", 1024 * 48, 2, 1), ctx(ctx) {
  accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);
  processSemaphore = std::make_shared<bell::WrappedSemaphore>();
  playableSemaphore = std::make_shared<bell::WrappedSemaphore>();

  // Start the task
  startTask();
};

TrackQueue::~TrackQueue() {
  stopTask();

  std::scoped_lock lock(tracksMutex);
}

void TrackQueue::runTask() {
  isRunning = true;

  std::scoped_lock lock(runningMutex);

  std::deque<std::shared_ptr<QueuedTrack>> trackQueue;

  while (isRunning) {
    if (processSemaphore->twait(200)) {
      if (!preloadedTracks.size())
        continue;
    }

    // Make sure we have the newest access key
    accessKey = accessKeyFetcher->getAccessKey();
    {
      std::scoped_lock lock(tracksMutex);

      trackQueue = preloadedTracks;
    }

    for (auto& track : trackQueue) {
      if (track) {
        if (this->processTrack(track))
          break;
      }
    }
  }
}

void TrackQueue::stopTask() {
  if (isRunning) {
    isRunning = false;
    processSemaphore->give();
    std::scoped_lock lock(runningMutex);
  }
}

std::shared_ptr<QueuedTrack> TrackQueue::consumeTrack(
    std::shared_ptr<QueuedTrack> prevTrack, int& offset) {
  std::scoped_lock lock(tracksMutex);

  if (!preloadedTracks.size()) {
    offset = -1;
    return nullptr;
  }

  auto prevTrackIter =
      std::find(preloadedTracks.begin(), preloadedTracks.end(), prevTrack);

  if (prevTrackIter != preloadedTracks.end()) {
    // Get offset of next track
    offset = prevTrackIter - preloadedTracks.begin() + 1;
  } else {
    offset = 0;
  }
  if (offset >= preloadedTracks.size()) {
    // Last track in preloaded queue
    return nullptr;
  }
  return preloadedTracks[offset];
}

bool TrackQueue::processTrack(std::shared_ptr<QueuedTrack> track) {
  switch (track->state) {
    case QueuedTrack::State::QUEUED:
      track->stepLoadMetadata(&track->pbTrack, &track->pbEpisode, tracksMutex,
                              processSemaphore);
      break;
    case QueuedTrack::State::KEY_REQUIRED:
      track->stepLoadAudioFile(tracksMutex, processSemaphore);
      break;
    case QueuedTrack::State::CDN_REQUIRED:
      track->stepLoadCDNUrl(accessKey);
    default:
      return false;
      // Do not perform any action
      break;
  }
  return true;
}
