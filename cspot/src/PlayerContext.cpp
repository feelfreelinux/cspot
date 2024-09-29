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

using namespace cspot;
char* PlayerContext::createStringReferenceIfFound(
    nlohmann::json::value_type& jsonObject, const char* key) {
  if (jsonObject.find(key) != jsonObject.end()) {
    std::string value = jsonObject.at(key).get<std::string>();
    if (value.size())
      return strdup(value.c_str());
  }
  return NULL;
}
void PlayerContext::resolveRadio(
    std::vector<std::pair<std::string, std::string>> metadata_map,
    void (*responseFunction)(void*), bool secondTry) {
  CSPOT_LOG(debug, "Resolve autoplay context: %s",
            secondTry ? tracks->at(0).uri : playerState->context_uri);
  std::string requestUrl =
      string_format("hm://autoplay-enabled/query?uri=%s",
                    secondTry ? tracks->at(0).uri : playerState->context_uri);
  auto responseHandler = [this, metadata_map, responseFunction,
                          secondTry](MercurySession::Response& res) {
    if (res.fail || !res.parts.size()) {
      if (secondTry)
        resolveRadio(metadata_map, responseFunction, true);
      else
        return responseFunction(NULL);
    }
    std::string resolve_autoplay =
        std::string(res.parts[0].begin(), res.parts[0].end());
    std::string requestUrl;
    if (radio_offset)
      requestUrl = (std::string)next_page_url;
    else {
      auto trackRef = tracks->end() - 1;
      while (
          trackRef != tracks->begin() &&
          (!trackRef->provider || strcmp(trackRef->provider, "context") != 0)) {
        trackRef--;
      }
      if (strcmp(trackRef->provider, "context") == 0)
        requestUrl = string_format(
            "hm://radio-apollo/v3/stations/%s?autoplay=true&offset=%i",
            &resolve_autoplay[0], trackRef->original_index);
      else {
        requestUrl = "hm://radio-apollo/v3/tracks/" +
                     (std::string)playerState->context_uri +
                     "?autoplay=true&count=50&isVideo=false&prev_tracks=";
        uint8_t copiedTracks = 0;
        while (trackRef != tracks->end()) {
          if (strcmp(trackRef->provider, "autoplay") == 0 &&
              (trackRef->uri && strrchr(trackRef->uri, ':'))) {
            if (copiedTracks)
              requestUrl += ",";
            requestUrl += (std::string)(strrchr(trackRef->uri, ':') + 1);
            copiedTracks++;
          }
          trackRef++;
        }
      }
    }
    auto responseHandler = [this, metadata_map,
                            responseFunction](MercurySession::Response& res) {
      if (res.fail || !res.parts.size())
        return responseFunction(NULL);
      if (!res.parts[0].size())
        return responseFunction(NULL);
      std::scoped_lock lock(trackListMutex);
      // remove old_tracks, keep 5 tracks in memory
      int remove_tracks = ((int)*index) - 5;
      if (remove_tracks > 0) {
        for (int i = 0; i < remove_tracks; i++) {
          if (tracks->at(i).full_metadata_count)
            tracks->at(i).metadata_count = tracks->at(i).full_metadata_count;
          pb_release(ProvidedTrack_fields, &tracks->at(i));
        }
        tracks->erase(tracks->begin(), tracks->begin() + remove_tracks);
      }
      *index = (uint8_t)(remove_tracks < 0 ? 5 + remove_tracks : 5);
      auto jsonResult = nlohmann::json::parse(res.parts[0]);
      if (jsonResult.find("uri") != jsonResult.end())
        context_uri = jsonResult.at("uri").get<std::string>();
      if (next_page_url != NULL)
        free(next_page_url);
      next_page_url = createStringReferenceIfFound(jsonResult, "next_page_url");
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
    ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                          responseHandler);
  };
  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
}

static unsigned long distributionToIndex(std::string d) {
  return strtoul(&d[d.find("(") + 1], nullptr, 10);
}

void PlayerContext::createIndexBasedOnTracklist(
    std::vector<ProvidedTrack>* tracks, nlohmann::json::value_type& json_tracks,
    bool shuffle, uint8_t page) {
  //create new index
  alternative_index.clear();
  std::vector<int64_t> shuffle_index;
  bool smart_shuffle =
      (json_tracks.at(0).find("metadata") == json_tracks.at(0).end() ||
       json_tracks.at(0).find("metadata")->find("shuffle.distribution") ==
           json_tracks.at(0).find("metadata")->end())
          ? false
          : true;
  for (int i = 0; i < tracks->size(); i++) {
    if (strstr(tracks->at(i).uri, "spotify:delimiter")) {
      uint8_t release_offset = 1;
      while (i + release_offset < tracks->size()) {
        cspot::TrackReference::pbReleaseProvidedTrack(
            std::addressof(tracks->at(i + release_offset)));
        release_offset++;
      }
      tracks->erase(tracks->begin() + i, tracks->end());
      break;
    }
  }
  if (smart_shuffle)
    alternative_index = std::vector<int64_t>(json_tracks.size());
  for (int i = 0; i < json_tracks.size(); i++) {
    if (smart_shuffle) {
      alternative_index[distributionToIndex(json_tracks.at(i)
                                                .find("metadata")
                                                ->find("shuffle.distribution")
                                                ->get<std::string>()) -
                        1] = i;
    } else if (!shuffle)
      alternative_index.push_back(i);
    for (auto& track : *tracks) {
      if (strcmp(track.uri,
                 json_tracks.at(i).at("uri").get<std::string>().c_str()) == 0) {
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
uint8_t PlayerContext::jsonToTracklist(
    std::vector<ProvidedTrack>* tracks,
    std::vector<std::pair<std::string, std::string>> metadata_map,
    nlohmann::json::value_type& json_tracks, const char* provider,
    int64_t offset, uint8_t page, bool shuffle, bool preloadedTrack) {
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
    int64_t index = radio ? offset : alternative_index[offset];
    if (index >= json_tracks.size() || index < 0) {
      offset++;
      continue;
    }
    auto track = json_tracks.at(index);
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
    if (track.find("metadata") != track.end()) {
      if (track.at("metadata").find("decision_id") !=
          track.at("metadata").end()) {
        new_track.metadata[metadata_offset].key = strdup("decision_id");
        new_track.metadata[metadata_offset].value =
            strdup(std::string(track.at("metadata").at("decision_id")).c_str());
        metadata_offset++;
      }
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
    new_track.original_index = index;
    new_track.page = page;
    tracks->push_back(new_track);
    copiedTracks++;
    offset++;
  }
  if (offset == json_tracks.size()) {
    ProvidedTrack new_track = ProvidedTrack_init_zero;
    new_track.uri = strdup("spotify:delimiter");
    new_track.uid = strdup("delimiter0");
    new_track.provider = strdup("context");
    new_track.removed = strdup("context/delimiter");
    metadata_map.insert(metadata_map.begin(), std::make_pair("hidden", "true"));
    metadata_map.push_back(
        std::make_pair("actions.skipping_next_past_track", "resume"));
    metadata_map.push_back(
        std::make_pair("actions.advancing_past_track", "resume"));
    metadata_map.push_back(std::make_pair("iteration", "0"));
    for (auto metadata : metadata_map) {
      new_track.metadata[new_track.metadata_count].key =
          strdup(metadata.first.c_str());
      new_track.metadata[new_track.metadata_count].value =
          strdup(metadata.second.c_str());
      new_track.metadata_count++;
    }
    tracks->push_back(new_track);
  }
  return copiedTracks;
}
void PlayerContext::resolveTracklist(
    std::vector<std::pair<std::string, std::string>> metadata_map,
    void (*responseFunction)(void*), bool changed_state) {
  //if last track was no radio track, resolve tracklist
  //CSPOT_LOG()
  CSPOT_LOG(debug, "Resolve tracklist");
  if (changed_state) {
    for (int i = 0; i < tracks->size(); i++) {
      if (strstr(tracks->at(i).uri, "spotify:delimiter")) {
        uint8_t release_offset = 0;
        while (i + release_offset < tracks->size()) {
          cspot::TrackReference::pbReleaseProvidedTrack(
              std::addressof(tracks->at(i + release_offset)));
          release_offset++;
        }
        tracks->erase(tracks->begin() + i, tracks->end());
        break;
      }
    }
  }
  if ((playerState->track.provider == NULL ||
       strcmp(playerState->track.provider, "autoplay")) != 0 &&
      playerState->context_uri != NULL) {
    std::string requestUrl = string_format(
        "hm://context-resolve/v1/%s",
        (playerState->options.shuffling_context && playerState->context_url)
            ? &playerState->context_url[10]
            : playerState->context_uri);
    auto responseHandler = [this, metadata_map, responseFunction,
                            changed_state](MercurySession::Response& res) {
      if (res.fail || !res.parts.size())
        return;
      if (!res.parts[0].size())
        return;
      auto jsonResult = nlohmann::json::parse(res.parts[0]);
      std::scoped_lock lock(trackListMutex);
      uint8_t copy_tracks = 0;
      if (tracks->size()) {
        // remove old_tracks, keep 5 tracks in memory
        int remove_tracks = ((int)*index) - 5;
        if (remove_tracks > 0) {
          for (int i = 0; i < remove_tracks; i++) {
            if (tracks->at(i).full_metadata_count >
                tracks->at(i).metadata_count)
              tracks->at(i).metadata_count = tracks->at(i).full_metadata_count;
            pb_release(ProvidedTrack_fields, &tracks->at(i));
          }
          tracks->erase(tracks->begin(), tracks->begin() + remove_tracks);
        }
        *index = (uint8_t)(remove_tracks < 0 ? 5 + remove_tracks : 5);

        auto trackref = tracks->end() - 1;
        //if last track was a queued track/delimiter, try to look for a normal track as lookup reference
        while (trackref != tracks->begin() &&
               (strcmp(trackref->provider, "context") != 0 ||
                trackref->removed != NULL)) {
          trackref--;
        }
        //if no normal track was found, resolve radio
        if (strcmp(trackref->provider, "queue") == 0)
          return resolveRadio(metadata_map, responseFunction);
      looking_for_playlisttrack:;
        //if last track was a smart_shuffled track
        if (trackref != tracks->begin()) {
          if (trackref->removed != NULL ||
              strcmp(trackref->provider, "context") !=
                  0) {  //is a delimiter || is queued
            trackref--;
            goto looking_for_playlisttrack;
          }
          for (int i = 0; i < trackref->full_metadata_count; i++)
            if (trackref->metadata[i].key &&
                strcmp(trackref->metadata[i].key, "provider") == 0 &&
                !playerState->options
                     .context_enhancement_count) {  //was a smart_shuffle-track, but smart_shuffle is no more
              trackref--;
              goto looking_for_playlisttrack;
            }
        }
        if (trackref == tracks->begin() &&
            strcmp(trackref->uri, "spotify:delimiter") == 0)
          return;
        //if track available were all smart_shuffle_tracks, load Tracklist from 0;
        if (trackref == tracks->begin()) {

          for (int i = 0;
               i < (trackref->full_metadata_count > trackref->metadata_count
                        ? trackref->full_metadata_count
                        : trackref->metadata_count);
               i++)
            if ((strcmp(trackref->metadata[i].key, "provider") == 0 &&
                 !playerState->options.context_enhancement_count)) {
              jsonToTracklist(tracks, metadata_map,
                              jsonResult["pages"][0]["tracks"], "context", 0, 0,
                              playerState->options.shuffling_context, false);
              return responseFunction(NULL);
            }
        }

        //look for trackreference
        for (int i = 0; i < jsonResult["pages"].size(); i++) {
          int64_t offset = 0;
          if (!copy_tracks) {
            for (auto track : jsonResult["pages"][i]["tracks"]) {
              if (strcmp(track["uri"].get<std::string>().c_str(),
                         trackref->uri) == 0) {
                copy_tracks = 1;
                break;
              }
              offset++;
            }
          }
          //if trackreference was found
          if (copy_tracks) {
            if (changed_state) {
              createIndexBasedOnTracklist(
                  tracks, jsonResult["pages"][i]["tracks"],
                  playerState->options.shuffling_context, i);
              if (jsonResult["pages"][i]["tracks"].at(0).find("metadata") !=
                      jsonResult["pages"][i]["tracks"].at(0).end() &&
                  jsonResult["pages"][i]["tracks"]
                          .at(0)
                          .find("metadata")
                          ->find("shuffle.distribution") !=
                      jsonResult["pages"][i]["tracks"]
                          .at(0)
                          .find("metadata")
                          ->end()) {
                if (playerState->options.shuffling_context) {
                  if (alternative_index[0] != offset) {
                    for (auto& index : alternative_index)
                      if (index == offset) {
                        index = alternative_index[0];
                        alternative_index[0] = offset;
                        break;
                      }
                  }
                }
              }
            }
            copy_tracks = jsonToTracklist(
                tracks, metadata_map, jsonResult["pages"][i]["tracks"],
                "context", offset, i, playerState->options.shuffling_context,
                true);
            if (copy_tracks)
              break;
          }
        }
      }
      if (!copy_tracks) {
        if (this->playerState->options.repeating_context || !tracks->size()) {
          if (*index >= tracks->size()) {
            for (int i = 0; i < tracks->size(); i++)
              cspot::TrackReference::pbReleaseProvidedTrack(&tracks->at(i));
            tracks->clear();
            *index = 0;
          } else
            *index = 1;
          createIndexBasedOnTracklist(tracks, jsonResult["pages"][0]["tracks"],
                                      playerState->options.shuffling_context,
                                      0);
          jsonToTracklist(tracks, metadata_map,
                          jsonResult["pages"][0]["tracks"], "context", 0, 0,
                          playerState->options.shuffling_context, false);
          playerState->track = tracks->back();

          if (*index >= tracks->size() && tracks->size()) {
            ProvidedTrack new_track = ProvidedTrack_init_zero;
            new_track.uri = strdup("spotify:delimiter");
            new_track.uid = strdup("uiddelimiter0");
            new_track.provider = strdup("context");
            new_track.removed = strdup("context/delimiter");
            new_track.metadata[new_track.metadata_count].key = strdup("hidden");
            new_track.metadata[new_track.metadata_count].value = strdup("true");
            new_track.metadata_count++;
            new_track.metadata[new_track.metadata_count].key =
                strdup("actions.skipping_next_past_track");
            new_track.metadata[new_track.metadata_count].value =
                strdup("resume");
            new_track.metadata_count++;
            new_track.metadata[new_track.metadata_count].key =
                strdup("actions.advancing_past_track");
            new_track.metadata[new_track.metadata_count].value =
                strdup("resume");
            new_track.metadata_count++;
            new_track.metadata[new_track.metadata_count].key =
                strdup("iteration");
            new_track.metadata[new_track.metadata_count].value = strdup("0");
            new_track.metadata_count++;
            for (auto metadata : metadata_map) {
              new_track.metadata[new_track.metadata_count].key =
                  strdup(metadata.first.c_str());
              new_track.metadata[new_track.metadata_count].value =
                  strdup(metadata.second.c_str());
              new_track.metadata_count++;
            }

            tracks->insert(tracks->begin(), new_track);
          }
        } else
          return resolveRadio(metadata_map, responseFunction);
      }
      responseFunction(NULL);
    };
    ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                          responseHandler);

  } else
    resolveRadio(metadata_map, responseFunction);
}