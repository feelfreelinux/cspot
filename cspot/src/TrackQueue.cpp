#include "TrackQueue.h"
#include <pb_decode.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <random>

#include "AccessKeyFetcher.h"
#include "BellTask.h"
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

static void randomizeIndex(std::vector<int32_t>& index, uint16_t offset,
                           std::default_random_engine rng) {
  std::shuffle(index.begin() + offset, index.end(), rng);
}

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

QueuedTrack::QueuedTrack(TrackReference& ref,
                         std::shared_ptr<cspot::Context> ctx,
                         uint32_t requestedPosition)
    : requestedPosition(requestedPosition), ctx(ctx) {
  this->ref = ref;

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
  AudioFile* selectedFiles = nullptr;

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

    if (trackId.size() > 0) {
      // Load track information
      trackInfo.loadPbTrack(pbTrack, trackId);
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

      // Load track information
      trackInfo.loadPbEpisode(pbEpisode, trackId);
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

  // Assign track identifier
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

  try {

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
  } catch (...) {
    CSPOT_LOG(error, "Cannot fetch CDN URL");
    state = State::FAILED;
    loadedSemaphore->give();
  }
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
    : bell::Task("CSpotTrackQueue", 1024 * 32, 2, 1),
      playbackState(state),
      ctx(ctx) {
  accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);
  processSemaphore = std::make_shared<bell::WrappedSemaphore>();
  playableSemaphore = std::make_shared<bell::WrappedSemaphore>();

  // Assign encode callback to track list
  playbackState->innerFrame.state.track.funcs.encode =
      &TrackReference::pbEncodeTrackList;
  playbackState->innerFrame.state.track.arg = &ghostTracks;
  pbTrack = Track_init_zero;
  pbEpisode = Episode_init_zero;

  // Start the task
  startTask();
};

TrackQueue::~TrackQueue() {
  stopTask();

  std::scoped_lock lock(tracksMutex);

  pb_release(Track_fields, &pbTrack);
  pb_release(Episode_fields, &pbEpisode);
}

TrackInfo TrackQueue::getTrackInfo(std::string_view identifier) {
  for (auto& track : preloadedTracks) {
    if (track->identifier == identifier)
      return track->trackInfo;
  }
  return TrackInfo{};
}

void TrackQueue::runTask() {
  isRunning = true;

  std::scoped_lock lock(runningMutex);

  std::deque<std::shared_ptr<QueuedTrack>> trackQueue;

  while (isRunning) {
    processSemaphore->twait(100);

    // Make sure we have the newest access key
    accessKey = accessKeyFetcher->getAccessKey();

    int loadedIndex = currentTracksIndex;

    // No tracks loaded yet
    if (loadedIndex < 0) {
      continue;
    } else {
      std::scoped_lock lock(tracksMutex);

      trackQueue = preloadedTracks;
    }

    for (auto& track : trackQueue) {
      if (track) {
        this->processTrack(track);
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

  if (currentTracksIndex == -1 || currentTracksIndex >= currentTracks.size()) {
    return nullptr;
  }

  // No previous track, return head
  if (prevTrack == nullptr || playbackState->innerFrame.state.repeat) {
    offset = 0;

    return preloadedTracks[0];
  }

  // if (currentTracksIndex + preloadedTracks.size() >= currentTracks.size()) {
  //   offset = -1;

  //   // Last track in queue
  //   return nullptr;
  // }

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

  // Return the current track
  return preloadedTracks[offset];
}

void TrackQueue::update_ghost_tracks(int16_t offset) {
  if (currentTracksIndex + offset >= SEND_OLD_TRACKS)
    this->playbackState->innerFrame.state.index = SEND_OLD_TRACKS;
  else
    this->playbackState->innerFrame.state.index = currentTracksIndex + offset;
  ghostTracks.clear();
  //add SEND_OLD_TRACKS already played tracks
  size_t ghostindex;
  size_t index = currentTracksIndex + offset;
  for (int i = 1; i <= this->playbackState->innerFrame.state.index; i++) {
    ghostindex = index - i;
    if (ghostindex < alt_index.size())
      ghostindex = alt_index[ghostindex];
    ghostTracks.push_back(currentTracks[ghostindex]);
  }
  if (queuedTracks.size() > 0) {
    if (preloadedTracks[0]->ref.gid != queuedTracks[0].second.gid) {
      ghostindex = index < alt_index.size() ? alt_index[index] : index;
      ghostTracks.push_back(currentTracks[ghostindex]);
      index++;
    }
    for (auto track : queuedTracks)
      ghostTracks.push_back(track.second);
  }
  ghostindex = index;
  while (ghostTracks.size() < SEND_OLD_TRACKS + CONFIG_UPDATE_FUTURE_TRACKS &&
         index < currentTracks.size()) {
    ghostindex = index < alt_index.size() ? alt_index[index] : index;
    ghostTracks.push_back(currentTracks[ghostindex]);
    index++;
  }
}

void TrackQueue::loadRadio(std::string req) {
  size_t keepTracks = currentTracks.size() - currentTracksIndex;
  keepTracks =
      keepTracks < inner_tracks_treshhold ? keepTracks : inner_tracks_treshhold;
  if (keepTracks < inner_tracks_treshhold && continue_with_radio &&
      currentTracks.size()) {

    //if currentTrack is over the treshhold, remove some tracks from the front
    if (currentTracks.size() > inner_tracks_treshhold * 3) {
      uint32_t int_offset = currentTracks.size() - inner_tracks_treshhold;
      currentTracks.erase(currentTracks.begin(),
                          currentTracks.begin() + int_offset);
      currentTracksIndex -= int_offset;
    }

    std::string requestUrl = string_format(
        "hm://radio-apollo/v3/stations/%s?autoplay=true&offset=%i", &req[0],
        radio_offset);

    auto responseHandler = [this](MercurySession::Response& res) {
      std::scoped_lock lock(tracksMutex);
      CSPOT_LOG(info, "Fetched new radio tracks");

      if (res.parts[0].size() == 0)
        return;
      auto jsonResult = nlohmann::json::parse(res.parts[0]);
      radio_offset += jsonResult["tracks"].size();
      std::string uri;
      for (int i = 0; i < jsonResult["tracks"].size(); i++) {
        currentTracks.push_back(
            TrackReference(jsonResult["tracks"][i]["original_gid"], "radio"));
      }
      this->ctx->playbackMetrics->correlation_id = jsonResult["correlation_id"];
      if (preloadedTracks.size() < MAX_TRACKS_PRELOAD) {
        queueNextTrack(preloadedTracks.size());
      }
    };
    // Execute the request
    ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                          responseHandler);
  }
}

void TrackQueue::resolveAutoplay() {
  if (!this->playbackState->innerFrame.state.context_uri)
    return;
  std::string requestUrl =
      string_format("hm://autoplay-enabled/query?uri=%s",
                    this->playbackState->innerFrame.state.context_uri);
  auto responseHandler = [this](MercurySession::Response& res) {
    if (res.parts.size()) {
      loadRadio(std::string(res.parts[0].begin(), res.parts[0].end()));
    }
  };
  // Execute the request
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}

void TrackQueue::resolveContext() {
  std::scoped_lock lock(tracksMutex);
  std::string requestUrl =
      string_format("hm://context-resolve/v1/%s",
                    this->playbackState->innerFrame.state.context_uri);
  auto responseHandler = [this](MercurySession::Response& res) {
    if (!res.parts.size())
      return;
    std::scoped_lock lock(tracksMutex);
    auto jsonResult = nlohmann::json::parse(res.parts[0]);
    //do nothing if last track is the same as last track in currentTracks
    for (auto pages_itr : jsonResult["pages"]) {
      TrackReference _ref;
      int32_t front = 0;
      if (pages_itr.find("tracks") == pages_itr.end())
        continue;
      if (this->playbackState->innerFrame.state.shuffle) {
        std::vector<int32_t> new_index = {};
        std::vector<TrackReference> new_tracks = {};
        for (const auto& track : pages_itr["tracks"]) {
          new_tracks.push_back(TrackReference(track["uri"]));
        }
        for (const auto& itr : currentTracks) {
          auto currentTracksIter =
              std::find(new_tracks.begin(), new_tracks.end(), itr);
          if (currentTracksIter != new_tracks.end()) {
            int32_t old_index =
                std::distance(new_tracks.begin(), currentTracksIter);
            new_index.push_back(old_index);
          } else {
            int newIndex = new_tracks.size();
            new_tracks.push_back(itr);
            new_index.push_back(newIndex);
          }
        }
        for (uint32_t i = 0; i < new_index.size(); i++)
          if (std::find(new_index.begin(), new_index.end(), i) ==
              new_index.end())
            new_index.push_back(i);
        randomizeIndex(new_index, currentTracks.size(), rng);
        alt_index = new_index;
        currentTracks = new_tracks;
      } else {
        auto altref = currentTracks.front().gid;
        uint8_t loop_pointer = 0;
        for (auto itr : pages_itr["tracks"]) {
          _ref = TrackReference(itr["uri"]);
          switch (loop_pointer) {
            case 0:
              if (_ref.gid == altref) {
                loop_pointer++;
                altref = currentTracks.back().gid;
                if (altref == base62Decode(pages_itr["tracks"].back()["uri"]))
                  goto exit_loop;
              } else {
                //for completion of the trasklist in case of shuffle, add missing item too the front
                currentTracks.insert(currentTracks.begin() + front, _ref);
                front++;
              }
              break;
            case 1:
              if (_ref.gid == altref)
                loop_pointer++;
              break;
            case 2:
              //add missing tracks to the back
              currentTracks.push_back(_ref);
              break;
            default:
              break;
          }
        }
      exit_loop:;
        currentTracksIndex += front;
        alt_index.clear();
        for (int32_t i = 0; i < currentTracks.size(); i++)
          alt_index.push_back(i);
      }
      currentTracksSize = currentTracks.size();
      CSPOT_LOG(info, "Resolved context, new currentTracks-size = %i",
                currentTracksSize);
    }
  };
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
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

      if (track->state == QueuedTrack::State::READY) {
        if (!context_resolved) {
          resolveContext();
          context_resolved = true;
        }
        if (continue_with_radio)
          if (preloadedTracks.size() + currentTracksIndex >=
              currentTracks.size())
            resolveAutoplay();
        if (preloadedTracks.size() < MAX_TRACKS_PRELOAD) {
          // Queue a new track to preload
          queueNextTrack(preloadedTracks.size());
        }
      }
      break;
    default:
      // Do not perform any action
      break;
  }
}

bool TrackQueue::queueNextTrack(int offset, uint32_t positionMs) {
  int requestedRefIndex = offset + currentTracksIndex;
  if (queuedTracks.size()) {
    for (auto track : queuedTracks) {
      if (preloadedTracks.size() >= MAX_TRACKS_PRELOAD)
        return true;
      for (auto loaded_track : preloadedTracks) {
        if (track.second.gid == loaded_track->ref.gid)
          goto loadedTrackAlready;
      }
      preloadedTracks.push_back(
          std::make_shared<QueuedTrack>(track.second, ctx, 0));
    loadedTrackAlready:;
    }
    if (preloadedTracks.size() >= MAX_TRACKS_PRELOAD)
      return true;
  }
  if (playbackState->innerFrame.state.shuffle &&
      requestedRefIndex < alt_index.size())
    requestedRefIndex = alt_index[requestedRefIndex];

  if (requestedRefIndex < 0 || requestedRefIndex >= currentTracks.size()) {
    return false;
  }

  // in case we re-queue current track, make sure position is updated (0)
  if (offset == 0 && preloadedTracks.size() &&
      preloadedTracks[0]->ref == currentTracks[currentTracksIndex]) {
    preloadedTracks.pop_front();
  }

  if (offset <= 0) {
    preloadedTracks.push_front(std::make_shared<QueuedTrack>(
        currentTracks[requestedRefIndex], ctx, positionMs));
  } else {
    preloadedTracks.push_back(std::make_shared<QueuedTrack>(
        currentTracks[requestedRefIndex], ctx, positionMs));
  }

  return true;
}

bool TrackQueue::skipTrack(SkipDirection dir, bool expectNotify) {
  bool skipped = true;
  std::scoped_lock lock(tracksMutex);

  if (dir == SkipDirection::PREV) {
    uint64_t position =
        !playbackState->innerFrame.state.has_position_ms
            ? 0
            : playbackState->innerFrame.state.position_ms +
                  ctx->timeProvider->getSyncedTimestamp() -
                  playbackState->innerFrame.state.position_measured_at;

    if (currentTracksIndex > 0 && position < 3000) {
      queueNextTrack(-1);

      if (preloadedTracks.size() > MAX_TRACKS_PRELOAD) {
        preloadedTracks.pop_back();
      }

      currentTracksIndex--;
    } else {
      queueNextTrack(0);
    }
  } else {
    if (currentTracks.size() > currentTracksIndex + 1) {
      if (queuedTracks.size() > 0) {
        if (preloadedTracks[0]->ref.gid == queuedTracks[0].second.gid) {
          queuedTracks.pop_front();
          preloadedTracks.pop_front();
          if (queuedTracks.size() <= 0) {
            queueNextTrack(1);
          }
        } else {
          preloadedTracks.pop_front();
          currentTracksIndex++;
        }
      } else {
        preloadedTracks.pop_front();
        if (!queueNextTrack(preloadedTracks.size() + 1)) {
          CSPOT_LOG(info, "Failed to queue next track");
        }
        currentTracksIndex++;
      }
    } else {
      skipped = false;
    }
  }

  if (skipped) {
    // Update frame data
    playbackState->innerFrame.state.playing_track_index = currentTracksIndex;

    if (expectNotify) {
      // Reset position to zero
      notifyPending = true;
    }
  }

  return skipped;
}

bool TrackQueue::hasTracks() {
  std::scoped_lock lock(tracksMutex);

  return currentTracks.size() > 0;
}

bool TrackQueue::isFinished() {
  std::scoped_lock lock(tracksMutex);
  return currentTracksIndex >= currentTracks.size() - 1;
}

void TrackQueue::shuffle_tracks(bool shuffleTracks) {
  std::scoped_lock lock(tracksMutex);
  if (!shuffleTracks)
    currentTracksIndex = alt_index[currentTracksIndex];
  alt_index.clear();
  for (int i = 0; i < currentTracksSize; i++)
    alt_index.push_back(i);
  if (shuffleTracks) {
    alt_index[currentTracksIndex] = 0;
    alt_index[0] = currentTracksIndex;
    randomizeIndex(alt_index, 1, rng);
    currentTracksIndex = 0;
  }
  preloadedTracks.erase(preloadedTracks.begin() + 1, preloadedTracks.end());
  // Push a song on the preloaded queue
  queueNextTrack(1, 0);
  update_ghost_tracks();
}

bool TrackQueue::updateTracks(uint32_t requestedPosition, bool initial) {
  std::scoped_lock lock(tracksMutex);
  bool cleared = true;

  if (initial) {
    // initialize new random_engine
    rng = std::default_random_engine{rd()};
    // Copy requested track list
    currentTracks = playbackState->remoteTracks;
    currentTracksIndex = playbackState->innerFrame.state.playing_track_index;
    currentTracksSize = currentTracks.size();

    // Revert the alternative Index to it's inital Index structure
    alt_index.clear();
    for (int i = 0; i < playbackState->remoteTracks.size(); i++)
      alt_index.push_back(i);

    // Clear preloaded tracks
    preloadedTracks.clear();

    if (currentTracksIndex < currentTracks.size()) {
      // Push a song on the preloaded queue
      queueNextTrack(0, requestedPosition);
    }

    // We already updated track meta, mark it
    notifyPending = true;
    radio_offset = 0;

    playableSemaphore->give();
  } else {
    queuedTracks.push_back(std::make_pair(
        currentTracksIndex + 1,
        playbackState->remoteTracks[playbackState->innerFrame.state.index + 1 +
                                    queuedTracks.size()]));
    preloadedTracks.erase(preloadedTracks.begin() + 1, preloadedTracks.end());
    queueNextTrack(1);
    cleared = false;
  }
  update_ghost_tracks();

  return cleared;
}
