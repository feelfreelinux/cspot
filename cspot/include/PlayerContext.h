#ifndef PLAYER_CONTEXT_H
#define PLAYER_CONTEXT_H

#include <deque>
#include <memory>   // for shared_ptr
#include <mutex>    //for scoped_loc, mutex...
#include <random>   //for random_device and default_random_engine
#include <utility>  // for pair
#include <vector>   //for vector

#include "BellTask.h"
#include "CSpotContext.h"
#include "TrackReference.h"  //for TrackReference

#include "protobuf/metadata.pb.h"  // for Track, _Track, AudioFile, Episode
#define MAX_TRACKS 80
namespace cspot {
class PlayerContext {
 public:
  PlayerContext(
      bell::Task* queue, std::shared_ptr<cspot::Context> ctx,
      std::vector<cspot::TrackReference>* track_list,
      std::deque<std::pair<int64_t, cspot::TrackReference>>* queued_list,
      std::mutex* trackMutex, uint32_t* index, bool shuffle, char* uri);
  ~PlayerContext();
  void resolveContext(uint32_t index, bool shuffle, char* uri);

 private:
  std::shared_ptr<cspot::Context> ctx;
  std::mutex* trackMutex;
  bell::Task* queue;
  std::shared_ptr<Frame> innerFrame;
  void shuffleIndex();
  void getTracks();
  std::vector<uint32_t> alternative_index;
  uint32_t* index;
  uint32_t last_index = 0;
  uint32_t radio_offset = 0;
  std::string context_uri;
  std::vector<cspot::TrackReference>* track_list;
  std::deque<std::pair<int64_t, cspot::TrackReference>>* queued_list;
  bool last_resolve_shuffled = false;
  bool resolveTracklist(uint32_t index, bool shuffle,
                        std::vector<uint8_t>& data);
  bool resolveInitial(uint32_t index, bool shuffle, std::vector<uint8_t>& data);
  bool resolveRadio(uint32_t index, std::vector<uint8_t>& data);
  std::random_device rd;
  std::default_random_engine rng;
};
};  // namespace cspot

#endif