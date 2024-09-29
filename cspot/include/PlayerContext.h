#ifndef PLAYER_CONTEXT_H
#define PLAYER_CONTEXT_H

#include <deque>
#include <memory>  // for shared_ptr
#include <mutex>   //for scoped_loc, mutex...
#include <mutex>
#include <random>   //for random_device and default_random_engine
#include <utility>  // for pair
#include <vector>   //for vector

#include "BellTask.h"
#include "CSpotContext.h"
#include "TrackReference.h"       //for TrackReference
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json

#include "protobuf/metadata.pb.h"  // for Track, _Track, AudioFile, Episode
#define MAX_TRACKS 80
namespace cspot {
struct PlayerContext {
  PlayerContext(std::shared_ptr<cspot::Context> ctx, PlayerState* playerState,
                std::vector<ProvidedTrack>* tracks, uint8_t* index) {
    this->ctx = ctx;
    this->playerState = playerState;
    this->tracks = tracks;
    this->index = index;
  }
  void resolveRadio(
      std::vector<std::pair<std::string, std::string>> metadata_map,
      void (*responseFunction)(void*), bool secondTry = false);

  void resolveTracklist(
      std::vector<std::pair<std::string, std::string>> metadata_map,
      void (*responseFunction)(void*), bool state_changed = false);
  uint8_t jsonToTracklist(
      std::vector<ProvidedTrack>* tracks,
      std::vector<std::pair<std::string, std::string>> metadata_map,
      nlohmann::json::value_type& json_tracks, const char* provider,
      int64_t offset = 0, uint8_t page = 0, bool shuffle = false,
      bool preloadedTrack = false);
  void createIndexBasedOnTracklist(std::vector<ProvidedTrack>* tracks,
                                   nlohmann::json::value_type& json_tracks,
                                   bool shuffle, uint8_t page);
  //void jsonToPlayerStateContext(PlayerState* playerState, std::vector<ProvidedTrack>* tracks, uint8_t* index, std::vector<std::pair<std::string,std::string>> metadata_map,nlohmann::json::object_t context);
  static char* createStringReferenceIfFound(
      nlohmann::json::value_type& jsonObject, const char* key);
  uint64_t radio_offset = 0;
  std::mutex trackListMutex;
  std::shared_ptr<cspot::Context> ctx;
  PlayerState* playerState;
  std::vector<ProvidedTrack>* tracks;
  uint8_t* index;
  std::vector<int64_t> alternative_index;
  char* next_page_url = NULL;
  std::string context_uri;
  ~PlayerContext() {
    if (next_page_url != NULL)
      free(next_page_url);
  }
};
};  // namespace cspot

#endif