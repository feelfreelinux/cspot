#include "PlayerContext.h"
#include "MercurySession.h"
#include "TrackQueue.h"

#ifdef BELL_ONLY_CJSON
#include "cJSON.h"
#else
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#endif

using namespace cspot;

PlayerContext::PlayerContext(
    bell::Task* queue, std::shared_ptr<cspot::Context> ctx,
    std::vector<cspot::TrackReference>* track_list,
    std::deque<std::pair<int64_t, cspot::TrackReference>>* queued_list,
    std::mutex* trackMutex, uint32_t* index, bool shuffle, char* uri) {
  this->queue = queue;
  this->ctx = ctx;
  this->track_list = track_list;
  this->trackMutex = trackMutex;
  this->queued_list = queued_list;
  this->index = index;
  rng = std::default_random_engine{rd()};
  std::string requestUrl = string_format("hm://context-resolve/v1/%s", uri);
  auto responseHandler = [this, shuffle](MercurySession::Response& res) {
    if (!res.parts.size())
      return;
    if (!res.parts[0].size())
      return;
    this->resolveInitial(*this->index, shuffle, res.parts[0]);
  };

  ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                        responseHandler);
  if (track_list->size() < 30)
    resolveContext(*this->index, shuffle, uri);
}

static void randomizeIndex(std::vector<uint32_t>& index, uint16_t offset,
                           std::default_random_engine rng) {
  std::shuffle(index.begin() + offset, index.end(), rng);
}

PlayerContext::~PlayerContext() {}

void PlayerContext::resolveContext(uint32_t index, bool shuffle, char* uri) {
  if ((last_index + track_list->size() >= alternative_index.size()) &&
      (last_resolve_shuffled == shuffle ||
       last_index + index >= alternative_index.size())) {
    std::string requestUrl =
        string_format("hm://autoplay-enabled/query?uri=%s", uri);
    auto responseHandler = [this, index](MercurySession::Response& res) {
      if (!res.parts.size())
        return;
      std::string resolve_autoplay =
          std::string(res.parts[0].begin(), res.parts[0].end());
      std::string requestUrl = string_format(
          "hm://radio-apollo/v3/stations/%s?autoplay=true&offset=%i",
          &resolve_autoplay[0], radio_offset);
      auto responseHandler = [this, index](MercurySession::Response& res) {
        if (!res.parts.size())
          return;
        if (!res.parts[0].size())
          return;
        this->resolveRadio(index, res.parts[0]);
      };
      ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                            responseHandler);
    };
    ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                          responseHandler);
  } else {
    std::string requestUrl = string_format("hm://context-resolve/v1/%s", uri);
    auto responseHandler = [this, index,
                            shuffle](MercurySession::Response& res) {
      if (!res.parts.size())
        return;
      if (!res.parts[0].size())
        return;
      this->resolveTracklist(index, shuffle, res.parts[0]);
    };

    ctx->session->execute(MercurySession::RequestType::GET, requestUrl,
                          responseHandler);
  }
}

bool PlayerContext::resolveRadio(uint32_t index, std::vector<uint8_t>& data) {
  std::scoped_lock lock(*trackMutex);
  index = *this->index;
  if (queued_list->size())
    if (queued_list->at(0).first == index || queued_list->at(0).first == -1)
      index--;
  auto jsonResult = nlohmann::json::parse(data);
  radio_offset += jsonResult["tracks"].size();
  track_list->erase(track_list->begin(), track_list->begin() + index);
  for (auto track : jsonResult["tracks"]) {
    track_list->push_back(TrackReference(track["uri"]));
  }
  this->last_index += index;
  this->ctx->playbackMetrics->correlation_id = jsonResult["correlation_id"];
  *this->index = 0;
  static_cast<cspot::TrackQueue*>(queue)->reloadTracks();
  return true;
}

bool PlayerContext::resolveTracklist(uint32_t index, bool shuffle,
                                     std::vector<uint8_t>& data) {
  index = *this->index;
  std::scoped_lock lock(*trackMutex);
  auto jsonResult = nlohmann::json::parse(data);
  if (queued_list->at(0).first == 0 || queued_list->at(0).first == -1)
    index--;
  for (uint32_t i = 0; i < queued_list->size(); i++)
    if (queued_list->at(i).first != -1)
      queued_list->at(i).first -= index;
  if (shuffle != last_resolve_shuffled) {
    if (shuffle) {
      alternative_index[0] = last_index + index;
      alternative_index[index] = 0;
      randomizeIndex(alternative_index, 1, rng);
      last_index = 0;
    } else {
      last_index = alternative_index[index + last_index];
      for (uint32_t i = 0; i < alternative_index.size(); i++)
        alternative_index[i] = i;
    }
  } else {
    last_index += index;
  }
  track_list->erase(track_list->begin(), track_list->begin() + index);
  track_list->erase(track_list->begin() + 1, track_list->end());
  uint32_t copyTracksSize = alternative_index.size() - last_index;
  for (int i = 1;
       i < (copyTracksSize < MAX_TRACKS ? copyTracksSize : MAX_TRACKS); i++) {
    track_list->push_back(
        TrackReference(jsonResult["pages"][0]["tracks"]
                                 [alternative_index[i + last_index]]["uri"]));
  }
  last_resolve_shuffled = shuffle;
  *this->index = 0;
  static_cast<cspot::TrackQueue*>(queue)->reloadTracks();
  return true;
}

bool PlayerContext::resolveInitial(uint32_t index, bool shuffle,
                                   std::vector<uint8_t>& data) {
  std::scoped_lock lock(*trackMutex);
  index = *this->index;
  auto jsonResult = nlohmann::json::parse(data);
  if (jsonResult.find("pages") == jsonResult.end())
    return false;
  // create a new_index based on track_list with all index -1
  std::vector<int64_t> new_index(track_list->size());
  std::fill(new_index.begin(), new_index.end(), -1);
  alternative_index = {};
  TrackReference ref;
  for (auto pages_itr : jsonResult["pages"]) {
    if (pages_itr.find("tracks") == pages_itr.end())
      continue;
    size_t alternative_offset = alternative_index.size();
    // add a index to alternative_index for each track in context
    for (uint32_t i = alternative_offset; i < pages_itr["tracks"].size(); i++)
      alternative_index.push_back(i);
    // find tracks from track_list
    for (const auto& track : pages_itr["tracks"]) {
      ref = TrackReference(track["uri"]);
      for (uint32_t i = 0; i < track_list->size(); i++) {
        if (ref.gid == track_list->data()[i].gid)
          new_index[i] = alternative_offset;
      }
      alternative_offset++;
    }
  }
  if (new_index[index] == -1)
    index--;
  for (int64_t i = 0; i < new_index.size(); i++) {
    if (new_index[i] == -1) {
      if (i >= index)
        queued_list->push_back(
            std::make_pair(i - index, track_list->data()[i]));
      track_list->erase(track_list->begin() + i);
      new_index.erase(new_index.begin() + i);
      i--;
    }
  }
  if (shuffle) {
    for (int i = 0; i < new_index.size(); i++) {
      if (new_index[i] >= new_index.size()) {
        alternative_index[new_index[i]] = alternative_index[i];
        alternative_index[i] = new_index[i];
      } else {
        *std::find(alternative_index.begin(), alternative_index.end(),
                   new_index[i]) = alternative_index[i];
        alternative_index[i] = new_index[i];
      }
    }
    randomizeIndex(alternative_index, new_index.size(), rng);
  }
  track_list->erase(track_list->begin(), track_list->begin() + index);
  last_index = shuffle ? index : new_index[index];
  last_resolve_shuffled = shuffle;
  *this->index = 0;
  //static_cast<cspot::TrackQueue*>(queue)->reloadTracks();
  return true;
}