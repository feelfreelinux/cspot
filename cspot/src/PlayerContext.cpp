#include "PlayerContext.h"
#include <cstring>
#include <utility>
#include <vector>
#include "MercurySession.h"
#include "protobuf/connect.pb.h"  // for PutStateRequest, DeviceState, PlayerState...

#include "BellLogger.h"  // for AbstractLogger
#include "Logger.h"      // for CSPOT_LOG
#ifdef BELL_ONLY_CJSON
#include "cJSON.h"
#else
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#endif

#define METADATA_STRING "metadata"
#define SMART_SHUFFLE_STRING "shuffle.distribution"

using namespace cspot;
/**
 * @brief Create a C string from a JSON object string value.
 *
 * @param[in] jsonObject The JSON object.
 * @param[in] key The key to look up.
 * @return The C string or NULL if the key isn't found or the value is empty.
 */
char* PlayerContext::createStringReferenceIfFound(
    nlohmann::json::value_type& jsonObject, const char* key) {
  auto object = jsonObject.find(key);
  if (object != jsonObject.end()) {
    std::string value = object.value();
    if (value.size())
      return strdup(value.c_str());
  }
  return NULL;
}

/**
 * @brief Retrieve metadata from a JSON object if a key is found.
 *
 * This function searches for a specified key in a JSON object. If the key is found,
 * it creates a metadata entry with the key and its corresponding value.
 *
 * @param[in] jsonObject The JSON object to search.
 * @param[in] key The key to look for in the JSON object.
 * @param[out] metadata The metadata entry to populate if the key is found.
 * @return True if the key is found and metadata is populated, false otherwise.
 */
bool createMetadataIfFound(nlohmann::json::value_type& jsonObject,
                           const char* key,
                           ProvidedTrack_MetadataEntry& metadata) {
  // Find the key in the JSON object
  auto object = jsonObject.find(key);

  // Check if the key exists in the JSON object
  if (object != jsonObject.end()) {
    std::string value = object.value();

    // Populate metadata with the key and its value
    metadata = {strdup(key), value.size() ? strdup(value.c_str()) : NULL};
    return true;
  }

  // Return false if the key is not found
  return false;
}

template <typename T>
T getFromJsonObject(nlohmann::json::value_type& jsonObject, const char* key) {
  T value;
  auto object = jsonObject.find(key);
  if (object != jsonObject.end())
    value = object.value();
  return value;
}

/**
 * @brief Query the autoplay service for a context.
 *
 * This function queries the autoplay service if a context is autoplay-enabled.
 * If the context is autoplay-enabled, it resolves the context into a tracklist.
 *
 * @param[in] metadata_map The metadata to pass to the autoplay service.
 * @param[in] responseFunction The function to call with the resolved tracklist.
 * @param[in] secondTry If true, use the first track in the tracklist as the context URI
 *                      instead of the context URI from the player state.
 */
// Helper function to split a string by a delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

// Helper function to join a vector of strings with a delimiter
std::string join(const std::vector<std::string>& vec, char delimiter) {
  std::ostringstream result;
  for (size_t i = 0; i < vec.size(); ++i) {
    result << vec[i];
    if (i < vec.size() - 1) {
      result << delimiter;
    }
  }
  return result.str();
}

// Function to process the next_page_url
char* processNextPageUrl(const std::string& url, size_t trackLimit,
                         uint64_t* radio_offset) {
  const std::string key = "prev_tracks=";
  size_t startPos = url.find(key);
  if (startPos == std::string::npos) {
    return NULL;  // No prev_tracks found
  }
  startPos += key.length();

  // Find the end of the prev_tracks parameter
  size_t endPos = url.find('&', startPos);
  std::string prevTracks = (endPos == std::string::npos)
                               ? url.substr(startPos)
                               : url.substr(startPos, endPos - startPos);

  // Split, cap, and join
  std::vector<std::string> tracks = split(prevTracks, ',');
  if (tracks.size() > trackLimit) {
    *radio_offset += tracks.size() - trackLimit;
    tracks.erase(tracks.begin(), tracks.end() - trackLimit);
  }
  std::string newPrevTracks = join(tracks, ',');

  // Rebuild the URL
  std::string rebuiltUrl = url.substr(0, startPos) + newPrevTracks +
                           "&offset=" + std::to_string(*radio_offset);
  return strdup(rebuiltUrl.c_str());
}
void PlayerContext::autoplayQuery(
    std::vector<std::pair<std::string, std::string>> metadata_map,
    void (*responseFunction)(void*), bool secondTry) {
  if (next_page_url != NULL)
    return resolveRadio(metadata_map, responseFunction, next_page_url);
  if (playerState->context_uri == NULL)
    secondTry = true;
  std::string requestUrl =
      string_format("hm://autoplay-enabled/query?uri=%s",
                    secondTry ? tracks->at(0).uri : playerState->context_uri);
  CSPOT_LOG(debug, "Querying autoplay: %s", &requestUrl[0]);
  auto responseHandler = [this, metadata_map, responseFunction,
                          secondTry](MercurySession::Response res) {
    if (res.fail || !res.parts.size() || !res.parts[0].size()) {
      if (!secondTry)
        return autoplayQuery(metadata_map, responseFunction, true);
      //else
      //return responseFunction((void*)radio_offset);
    }
    std::string resolve_autoplay =
        std::string(res.parts[0].begin(), res.parts[0].end());
    std::string requestUrl;
    {
      if (tracks->back().provider &&
          (strcmp(tracks->back().provider, "context") == 0 ||
           playerState->context_uri == NULL))
        requestUrl = string_format(
            "hm://radio-apollo/v3/stations/%s?autoplay=true",  //&offset=%i",
            &resolve_autoplay[0]);  //, tracks->back().original_index);
      else {
        requestUrl = "hm://radio-apollo/v3/tracks/" +
                     (std::string)tracks->at(0).uri +
                     "?autoplay=true&count=50&isVideo=false&prev_tracks=";
        bool copiedTracks = false;
        auto trackRef =
            tracks->size() > 50 ? tracks->end() - 50 : tracks->begin();
        while (trackRef != tracks->end()) {
          if (trackRef->removed == NULL &&  //is no demlimiter
              (trackRef->uri && strrchr(trackRef->uri, ':'))) {
            if (copiedTracks)
              requestUrl += ",";
            requestUrl += (std::string)(strrchr(trackRef->uri, ':') + 1);
            copiedTracks = true;
          }
          trackRef++;
        }
      }
      resolveRadio(metadata_map, responseFunction, &requestUrl[0]);
    }
  };
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}

void PlayerContext::resolveRadio(
    std::vector<std::pair<std::string, std::string>> metadata_map,
    void (*responseFunction)(void*), char* url) {
  CSPOT_LOG(debug, "Resolve radio");
  auto responseHandler = [this, metadata_map,
                          responseFunction](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return responseFunction((void*)radio_offset);
    if (!res.parts[0].size())
      return responseFunction((void*)radio_offset);
    // remove old_tracks, keep 5 tracks in memory
    if (*index > 5) {
      cspot::TrackReference::deleteTracksInRange(tracks, 0, *index - 5);
      *index = 4;
    }
    if (!nlohmann::json::accept(res.parts[0])) {
      return responseFunction((void*)radio_offset);
    }
    auto jsonResult = nlohmann::json::parse(res.parts[0]);
    context_uri = jsonResult.value("uri", context_uri);
    if (next_page_url != NULL)
      free(next_page_url);

    auto urlObject = jsonResult.find("next_page_url");
    if (urlObject != jsonResult.end()) {
      next_page_url = processNextPageUrl(urlObject.value(), 100, &radio_offset);
    }

    std::vector<std::pair<std::string, std::string>> metadata = metadata_map;
    metadata.push_back(std::make_pair("context_uri", context_uri));
    metadata.push_back(std::make_pair("entity_uri", context_uri));
    metadata.push_back(std::make_pair("iteration", "0"));
    metadata.insert(metadata.begin(),
                    std::make_pair("autoplay.is_autoplay", "true"));
    metadata.push_back(std::make_pair("track_player", "audio"));
    metadata.push_back(
        std::make_pair("actions.skipping_next_past_track", "resume"));
    metadata.push_back(
        std::make_pair("actions.skipping_prev_past_track", "resume"));
    jsonToTracklist(tracks, metadata, jsonResult["tracks"], "autoplay", 0);
    radio_offset++;
    responseFunction(NULL);
  };
  ctx->session->execute(MercurySession::RequestType::GET, url, responseHandler);
}

static unsigned long distributionToIndex(std::string d) {
  return strtoul(&d[d.find("(") + 1], nullptr, 10);
}

void PlayerContext::createIndexBasedOnTracklist(
    std::vector<ProvidedTrack>* tracks, nlohmann::json::value_type& json_tracks,
    bool shuffle, uint8_t page) {
  //create new index
  alternative_index.clear();
  std::vector<uint32_t> shuffle_index;
  bool smart_shuffle =
      (json_tracks.at(0).find(METADATA_STRING) == json_tracks.at(0).end() ||
       json_tracks.at(0).find(METADATA_STRING)->find(SMART_SHUFFLE_STRING) ==
           json_tracks.at(0).find(METADATA_STRING)->end())
          ? false
          : true;
  for (int i = 0; i < tracks->size(); i++) {
    if (strstr(tracks->at(i).uri, "spotify:delimiter")) {
      uint8_t release_offset = 1;
      CSPOT_LOG(info, "deleting %i tracks", tracks->size() - (i + 1));
      cspot::TrackReference::deleteTracksInRange(tracks, i + 1,
                                                 tracks->size() - 1);
      break;
    }
  }
  if (smart_shuffle)
    alternative_index = std::vector<uint32_t>(json_tracks.size());
  for (int i = 0; i < json_tracks.size(); i++) {
    if (smart_shuffle) {
      alternative_index[distributionToIndex(json_tracks.at(i)
                                                .find(METADATA_STRING)
                                                ->find(SMART_SHUFFLE_STRING)
                                                ->get<std::string>()) -
                        1] = i;
    } else if (!shuffle)
      alternative_index.push_back(i);
    for (auto& track : *tracks) {
      if (track.uri == json_tracks.at(i)["uri"].get_ref<const std::string&>()) {
        track.original_index = i;
        track.page = page;
        if (shuffle && !smart_shuffle)
          alternative_index.push_back(i);
        goto found_track;
      }
    }
    if (shuffle && !smart_shuffle)
      shuffle_index.push_back(i);
  found_track:;
  }
  if (shuffle && !smart_shuffle) {
    if (shuffle_index.size())
      ctx->rng = std::default_random_engine{ctx->rd()};
    std::shuffle(shuffle_index.begin(), shuffle_index.end(), ctx->rng);
    alternative_index.insert(strstr(tracks->back().uri, "spotify:delimiter")
                                 ? alternative_index.end()
                                 : alternative_index.begin(),
                             shuffle_index.begin(), shuffle_index.end());
  }
}
void jsonToDevice() {}

uint8_t PlayerContext::jsonToTracklist(
    std::vector<ProvidedTrack>* tracks,
    std::vector<std::pair<std::string, std::string>> metadata_map,
    nlohmann::json::value_type& json_tracks, const char* provider,
    uint32_t offset, uint8_t page, bool shuffle, bool preloadedTrack) {
  if (offset >= json_tracks.size())
    return 0;
  bool radio = (strcmp("autoplay", provider) == 0) ? true : false;
  uint8_t copiedTracks = 0;
  if (!radio && json_tracks.size() != alternative_index.size())
    createIndexBasedOnTracklist(tracks, json_tracks, shuffle, page);
  if (shuffle) {
    for (int i = 0; i < alternative_index.size(); i++)
      if (alternative_index[i] == offset) {
        offset = i;
        break;
      }
  }
  if (preloadedTrack)
    offset++;
  while (tracks->size() < MAX_TRACKS && offset < json_tracks.size()) {

    ProvidedTrack new_track = ProvidedTrack_init_zero;
    int64_t index_ = radio ? offset : alternative_index[offset];
    if (index_ >= json_tracks.size() || index_ < 0) {
      offset++;
      continue;
    }
    auto track = json_tracks.at(index_);
    new_track.uri = createStringReferenceIfFound(track, "uri");
    new_track.uid = createStringReferenceIfFound(track, "uid");
    new_track.provider = strdup(provider);
    uint8_t metadata_offset = 0;
    for (auto metadata : metadata_map) {
      new_track.metadata[metadata_offset].key = strdup(metadata.first.c_str());
      new_track.metadata[metadata_offset].value =
          strdup(metadata.second.c_str());
      metadata_offset++;
    }
    auto json_metadata = track.find(METADATA_STRING);
    if (json_metadata != track.end()) {
      metadata_offset += createMetadataIfFound(
          *json_metadata, "decision_id", new_track.metadata[metadata_offset]);
      //metadata_offset += createMetadataIfFound(*json_metadata, "provider", new_track.metadata[metadata_offset]);
      new_track.metadata_count = metadata_offset;
      for (auto metadata : track.at("metadata").items()) {
        if (metadata.key() != "decision_id" &&
            metadata.key() != "is_promotional" &&
            metadata.key() != "is_explicit") {
          new_track.metadata[metadata_offset].key =
              strdup(metadata.key().c_str());
          new_track.metadata[metadata_offset].value =
              strdup(((std::string)metadata.value()).c_str());
          metadata_offset++;
        }
      }
    }
    new_track.full_metadata_count = metadata_offset;
    if (!radio)
      new_track.metadata_count = metadata_offset;
    new_track.original_index = index_;
    new_track.page = page;
    tracks->push_back(new_track);
    copiedTracks++;
    offset++;
  }
  if (offset == json_tracks.size() && !radio) {
    ProvidedTrack new_track = ProvidedTrack_init_zero;
    new_track.uri = strdup("spotify:delimiter");
    new_track.uid = strdup("delimiter0");
    new_track.provider = strdup(provider);
    new_track.removed = strdup((std::string(provider) + "/delimiter").c_str());
    new_track.metadata[0] = {strdup("hidden"), strdup("true")};
    new_track.metadata[1] = {strdup("actions.skipping_next_past_track"),
                             strdup("resume")};
    new_track.metadata[2] = {strdup("actions.advancing_past_track"),
                             strdup("resume")};
    new_track.metadata_count = 3;
    new_track.full_metadata_count = 3;
    tracks->push_back(new_track);
    CSPOT_LOG(debug, "Adding delimiter to tracklist");
  }
  return copiedTracks;
}

void PlayerContext::resolveTracklist(
    std::vector<std::pair<std::string, std::string>> metadata_map,
    void (*responseFunction)(void*), bool changed_state,
    bool trackIsPartOfContext) {
  if (changed_state) {
    //new Playlist/context was loaded, check if there is a delimiter in tracklist and if, delete all after
    for (int i = 0; i < tracks->size(); i++) {
      if (tracks->at(i).uri && strstr(tracks->at(i).uri, "spotify:delimiter")) {
        CSPOT_LOG(debug,
                  "Deleting all tracks after delimiter, current tracklist "
                  "size: %i, index: %i",
                  tracks->size(), i);
        cspot::TrackReference::deleteTracksInRange(tracks, i,
                                                   tracks->size() - 1);
        break;
      }
    }
  }
  //if current track's provider is autoplay, skip loading the tracklist and query autoplay
  if (playerState->track.provider == NULL ||
      strcmp(playerState->track.provider, "autoplay") == 0) {
    return autoplayQuery(metadata_map, responseFunction);
  } else
    radio_offset = 0;
  if (playerState->context_uri == NULL)
    return responseFunction((void*)radio_offset);
  //if last track was no radio track, resolve tracklist

  std::string requestUrl = "hm://context-resolve/v1/%s";
  if (playerState->options.shuffling_context &&
      playerState->options.context_enhancement_count)
    requestUrl = string_format(requestUrl, &playerState->context_url[10]);
  else
    requestUrl = string_format(requestUrl, playerState->context_uri);
  CSPOT_LOG(debug, "Resolve context, url: %s", &requestUrl[0]);

  auto responseHandler = [this, metadata_map, responseFunction, changed_state,
                          trackIsPartOfContext](MercurySession::Response res) {
    if (res.fail || !res.parts.size())
      return responseFunction((void*)radio_offset);
    if (!res.parts[0].size())
      return responseFunction((void*)radio_offset);
    auto jsonResult = nlohmann::json::parse(res.parts[0]);
    uint8_t pageIndex = 0;
    uint32_t offset = 0;
    bool smartShuffledTrack = false, foundTrack = false;
    std::vector<ProvidedTrack>::iterator trackref = tracks->begin();
    if (tracks->size()) {
      // do all the look up magic before deleting tracks
      trackref = tracks->end() - 1;
      //if last track in tracklist was a queued track/delimiter, try to look for a normal track as lookup reference
      while (trackref != tracks->begin()) {
        smartShuffledTrack = false;
        if (trackref->removed == NULL) {  // is not a delimiter
          if (strcmp(trackref->provider, "context") == 0) {
            for (int i = 0; i < trackref->full_metadata_count; i++) {
              if (strcmp(trackref->metadata[i].key, "provider") == 0 &&
                  !playerState->options
                       .context_enhancement_count) {  //was a smart_shuffle-track, but smart_shuffle is no more
                smartShuffledTrack = true;
                break;
              }
            }
            break;
          }
        }
        CSPOT_LOG(debug, "trackref: %s", trackref->uri);
        trackref--;
      }
      if (trackref->removed != NULL) {
        if (tracks->size() == 1)
          return responseFunction((void*)radio_offset);
        else
          return autoplayQuery(metadata_map, responseFunction);
      }
      CSPOT_LOG(debug, "Last track in tracklist: %s", trackref->uri);
      if (!smartShuffledTrack ||
          playerState->options.context_enhancement_count) {
        for (pageIndex = 0; pageIndex < jsonResult["pages"].size();
             pageIndex++) {
          offset = 0;
          for (auto track : jsonResult["pages"][pageIndex]["tracks"]) {
            if (strcmp(track["uri"].get<std::string>().c_str(),
                       trackref->uri) == 0) {
              foundTrack = true;
              break;
            }
            //??if(foundTrack) break;
            if (foundTrack)
              break;
            offset++;
          }
          if (foundTrack)
            break;
        }
        //if trackreference was found
      }
    }
    if (!foundTrack) {
      pageIndex = 0;
      offset = 0;
    }
    CSPOT_LOG(debug, "Context at page %i, offset %i", pageIndex, offset);
    //delete tracks ?
    //if tracklist is in a new state, create index based on tracklist
    if (changed_state) {
      createIndexBasedOnTracklist(
          tracks, jsonResult["pages"][pageIndex]["tracks"],
          playerState->options.shuffling_context, pageIndex);

      //if smart_shuffle is tur
      if (playerState->options.shuffling_context) {
        if (alternative_index[trackref - tracks->begin()] != offset) {
          for (auto& index_ : alternative_index)
            if (index_ == offset) {
              index_ = alternative_index[trackref - tracks->begin()];
              alternative_index[trackref - tracks->begin()] = offset;
              break;
            }
        }
      }
    }

    // remove played tracks, keep 5 tracks in memory
    if (*index > 5) {
      cspot::TrackReference::deleteTracksInRange(tracks, 0, *index - 5);
      *index = 4;
    }
    CSPOT_LOG(
        debug,
        "Current tracklist size: %i, loading tracklist from page %i, offset %i",
        tracks->size(), pageIndex, offset);

    offset = jsonToTracklist(
        tracks, metadata_map, jsonResult["pages"][pageIndex]["tracks"],
        "context", offset, pageIndex, playerState->options.shuffling_context,
        foundTrack);
    if (offset > 1) {
      CSPOT_LOG(debug, "Tracklist populated with %i tracks", offset);
      return responseFunction(NULL);
    } else if (playerState->options.repeating_context) {
      jsonToTracklist(tracks, metadata_map,
                      jsonResult["pages"][pageIndex]["tracks"], "context", 0,
                      pageIndex, playerState->options.shuffling_context);
    } else
      return autoplayQuery(metadata_map, responseFunction);
  };
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}