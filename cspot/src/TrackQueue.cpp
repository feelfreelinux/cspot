#include "TrackQueue.h"
#include <pb_decode.h>

#include <memory>
#include <mutex>

#include "AccessKeyFetcher.h"
#include "BellTask.h"
#include "CDNAudioFile.h"
#include "CSpotContext.h"
#include "HTTPClient.h"
#include "Logger.h"
#include "WrappedSemaphore.h"
#include "cJSON.h"
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
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

QueuedTrack::QueuedTrack(TrackRef* pendingRef,
                         std::shared_ptr<cspot::Context> ctx,
                         uint32_t requestedPosition)
    : ctx(ctx), requestedPosition(requestedPosition) {
  // Prepare the track data
  ref = TrackReference::fromTrackRef(pendingRef);
  loadedSemaphore = std::make_shared<bell::WrappedSemaphore>();
  state = State::QUEUED;
}

QueuedTrack::~QueuedTrack() {
  state = State::FAILED;
  loadedSemaphore->give();

  if (pendingMercuryRequest != 0) {
    ctx->session->unregister(pendingMercuryRequest);
  }

  if (pendingAudioKeyRequest != 0) {
    ctx->session->unregisterAudioKey(pendingAudioKeyRequest);
  }
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
  AudioFile* selectedFiles;

  const char* countryCode = ctx->config.countryCode.c_str();

  if (ref.type == TrackReference::Type::TRACK) {
    CSPOT_LOG(info, "Track name: %s", pbTrack->name);
    CSPOT_LOG(info, "Track duration: %d", pbTrack->duration);

    CSPOT_LOG(debug, "trackInfo.restriction.size() = %d",
              pbTrack->restriction_count);

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
  } else {
    // Handle episodes
    CSPOT_LOG(info, "Episode name: %s", pbEpisode->name);
    CSPOT_LOG(info, "Episode duration: %d", pbEpisode->duration);

    CSPOT_LOG(debug, "episodeInfo.restriction.size() = %d",
              pbEpisode->restriction_count);

    // Check if we can play the episode
    if (!TrackDataUtils::doRestrictionsApply(pbEpisode->restriction,
                                             pbEpisode->restriction_count,
                                             countryCode)) {
      selectedFiles = pbEpisode->file;
      filesCount = pbEpisode->file_count;
      trackId = pbArrayToVector(pbEpisode->gid);
    }
  }

  // Find playable file
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
    state = State::FAILED;
    loadedSemaphore->give();
    return;
  }

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
        } else {
          CSPOT_LOG(error, "Failed to get audio key");
          state = State::FAILED;
          loadedSemaphore->give();
        }
        updateSemaphore->give();
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

  std::string requestUrl = string_format(
      "https://api.spotify.com/v1/storage-resolve/files/audio/interactive/"
      "%s?alt=json&product=9",
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

  CSPOT_LOG(info, "Received CDN URL, %s", cdnUrl.c_str());
  state = State::READY;
  loadedSemaphore->give();
}

void QueuedTrack::expire() {
  if (state != State::QUEUED) {
    state = State::FAILED;
    loadedSemaphore->give();
  }
}

void QueuedTrack::stepLoadMetadata(
    Track* pbTrack, Episode* pbEpisode, std::mutex& trackListMutex,
    std::shared_ptr<bell::WrappedSemaphore> updateSemaphore) {
  // Request metadata
  // accessKeyFetcher->getAccessKey(
  //     [this](std::string key) { this->accessKey = key; });

  // Prepare request ID
  std::string requestUrl = string_format(
      "hm://metadata/3/%s/%s",
      ref.type == TrackReference::Type::TRACK ? "track" : "episode",
      bytesToHexString(ref.gid).c_str());

  auto responseHandler = [this, pbTrack, pbEpisode, &trackListMutex,
                          updateSemaphore](MercurySession::Response& res) {
    std::scoped_lock lock(trackListMutex);

    if (res.parts.size() == 0) {
      // Invalid metadata, cannot proceed
      state = State::FAILED;
      updateSemaphore->give();
      loadedSemaphore->give();
      return;
    }

    // Parse the metadata
    if (ref.type == TrackReference::Type::TRACK) {
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
  pendingMercuryRequest = ctx->session->execute(
      MercurySession::RequestType::GET, requestUrl, responseHandler);

  // Set the state to pending
  state = State::PENDING_META;
}

TrackQueue::TrackQueue(std::shared_ptr<cspot::Context> ctx,
                       std::shared_ptr<cspot::PlaybackState> state)
    : bell::Task("CSpotTrackQueue", 1024, 0, 0),
      playbackState(state),
      ctx(ctx) {
  accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);
  processSemaphore = std::make_shared<bell::WrappedSemaphore>();
  playableSemaphore = std::make_shared<bell::WrappedSemaphore>();

  // Start the task
  startTask();
};

TrackQueue::~TrackQueue() {
  stopTask();

  for (auto& track : tracks) {
    track->expire();
  }

  // Clear the tracks
  tracks.clear();

  pb_release(Track_fields, &pbTrack);
  pb_release(Episode_fields, &pbEpisode);
}

void TrackQueue::runTask() {
  isRunning = true;

  std::scoped_lock lock(runningMutex);

  while (isRunning) {
    processSemaphore->twait(100);

    // Make sure we have the newest access key
    accessKey = accessKeyFetcher->getAccessKey();

    if (currentTracksIndex == -1) {
      continue;
    }

    for (int x = 0; x < MAX_TRACKS_PRELOAD; x++) {
      if (x + currentTracksIndex < tracks.size()) {
        this->processTrack(tracks[x + currentTracksIndex]);
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

std::shared_ptr<QueuedTrack> TrackQueue::consumeTrack(int offset) {
  std::scoped_lock lock(tracksMutex);

  if (currentTracksIndex == -1) {
    return nullptr;
  }

  if (currentTracksIndex + offset < 0 ||
      currentTracksIndex + offset >= tracks.size()) {
    return nullptr;
  }

  // Return the current track
  return tracks[currentTracksIndex + offset];
}

void TrackQueue::processTrack(std::shared_ptr<QueuedTrack> track) {
  switch (track->state) {
    case QueuedTrack::State::QUEUED:
      track->stepLoadMetadata(&pbTrack, &pbEpisode, tracksMutex,
                              processSemaphore);
      break;
    case QueuedTrack::State::KEY_REQUIRED:
      track->stepLoadAudioFile(tracksMutex, processSemaphore);
      break;
    case QueuedTrack::State::CDN_REQUIRED:
      track->stepLoadCDNUrl(accessKey);

      if (tracks[currentTracksIndex] == track && track->state == QueuedTrack::State::READY) {
        // First track in queue loaded, notify the playback state
        if (onPlaybackEvent) {
          onPlaybackEvent(PlaybackEvent::FIRST_LOADED, track);
        }
      }
      break;
    default:
      // Do not perform any action
      break;
  }
}

bool TrackQueue::hasTracks() {
  return tracks.size() > 0;
}

int TrackQueue::getTrackRelativePosition(std::shared_ptr<QueuedTrack> track) {
  std::scoped_lock lock(tracksMutex);

  if (currentTracksIndex == -1) {
    return -1;
  }

  for (int x = 0; x < tracks.size(); x++) {
    if (tracks[x] == track) {
      return x - currentTracksIndex;
    }
  }

  return -1;
}

void TrackQueue::updateTracks(uint32_t requestedPosition) {
  std::scoped_lock lock(tracksMutex);

  // For each track
  for (auto track : tracks) {
    track->expire();
  }

  // Clear the tracks vector
  tracks.clear();

  for (int x = 0; x < playbackState->innerFrame.state.track_count; x++) {
    // Create a new QueuedTrack object and add it to the tracks vector
    tracks.push_back(
        std::make_shared<QueuedTrack>(&playbackState->innerFrame.state.track[x],
                                      ctx, x == 0 ? requestedPosition : 0));
  }

  currentTracksIndex = playbackState->innerFrame.state.playing_track_index;

  playableSemaphore->give();
}