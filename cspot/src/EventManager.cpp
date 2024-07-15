#include "EventManager.h"
#include "CSpotContext.h"  // for Context::ConfigState, Context (ptr o...
#include "Logger.h"        // for CSPOT_LOG
#include "TrackQueue.h"    

using namespace cspot;

static std::string reason_text[11] = {
    "trackdone", "trackerror", "fwdbtn",  "backbtn", "endplay", "playbtn",
    "clickrow",  "logout",     "appload", "remote",  ""};

TrackMetrics::TrackMetrics(std::shared_ptr<cspot::Context> ctx, uint64_t pos) {
  this->ctx = ctx;
  timestamp = ctx->timeProvider->getSyncedTimestamp();
  currentInterval = std::make_shared<TrackInterval>(timestamp, pos);
}

void TrackMetrics::endInterval(uint64_t pos) {
  // end Interval
  currentInterval->length =
      this->ctx->timeProvider->getSyncedTimestamp() - currentInterval->start;
  currentInterval->end = currentInterval->position + currentInterval->length;
  totalAmountPlayed += currentInterval->length;
  // add skipped time
  if (pos != 0) {
    if (pos > currentInterval->position + currentInterval->length)
      skipped_forward.add(
          pos - (currentInterval->position + currentInterval->length));
    else
      skipped_backward.add(
          (currentInterval->position + currentInterval->length) - pos);
  }
  if (currentInterval->length > longestInterval)
    longestInterval = currentInterval->length;
  intervals =
      addInterval(intervals, {currentInterval->position, currentInterval->end});
}

uint64_t TrackMetrics::getPosition() {
  return (
      currentInterval->position +
      (this->ctx->timeProvider->getSyncedTimestamp() - currentInterval->start));
}

void TrackMetrics::startTrackDecoding() {
  trackHeaderTime = this->ctx->timeProvider->getSyncedTimestamp();
  audioKeyTime = trackHeaderTime - timestamp;
}

void TrackMetrics::startTrack(uint64_t pos) {
  audioKeyTime = this->ctx->timeProvider->getSyncedTimestamp() - audioKeyTime;
  currentInterval = std::make_shared<TrackInterval>(
      this->ctx->timeProvider->getSyncedTimestamp(), pos);
}

void TrackMetrics::endTrack() {
  endInterval(0);
  for (const auto& interval : intervals)
    totalMsOfPlayedTrack += interval.second - interval.first;
  totalAmountOfPlayTime =
      this->ctx->timeProvider->getSyncedTimestamp() - timestamp;
}

void TrackMetrics::newPosition(uint64_t pos) {
  if (pos == currentInterval->position + currentInterval->length)
    return;
  endInterval(pos);
  currentInterval = std::make_shared<TrackInterval>(
      this->ctx->timeProvider->getSyncedTimestamp(), pos);
}

std::string PlaybackMetrics::get_source_from_context() {
  if (context_uri != "") {
    if (context_uri.find("playlist") != std::string::npos)
      return "playlist";
    else if (context_uri.find("collection") != std::string::npos)
      return "your_library";
    else if (context_uri.find("artist") != std::string::npos)
      return "artist";
    else if (context_uri.find("album") != std::string::npos)
      return "album";
    else if (context_uri.find("track") != std::string::npos)
      return "track";
  }
  return "";
}
std::string PlaybackMetrics::get_end_source() {
  end_source = get_source_from_context();
  return end_source;
}
std::vector<uint8_t> PlaybackMetrics::sendEvent(
    std::shared_ptr<cspot::QueuedTrack> track) {
  std::string data;
  std::vector<uint8_t> msg;
  CSPOT_LOG(debug, "Sending playbackend event");
  std::string requestUrl = "hm://event-service/v1/events";

  append(&msg, "12");
  append(&msg, "45");
  append(&msg, std::to_string(seqNum++));
  append(&msg, this->ctx->config.deviceId);
  append(&msg, track->identifier);                   //PLAYBACKiD
  append(&msg, "00000000000000000000000000000000");  //parent_playback_id
  append(&msg, start_source);
  append(
      &msg,
      reason_text[(uint8_t)start_reason]);  //remote when taken over by anyone
  append(
      &msg,
      end_source == ""
          ? get_end_source()
          : end_source);  //free-tier-artist / playlist / your_library .com.spotify.gaia / artist
  append(&msg,
         reason_text[(uint8_t)end_reason]);  //remote when taken over of anyone
  append(
      &msg,
      std::to_string(
          track
              ->written_bytes));  //@librespot usually the same size as track->size, if shifted forward, usually shorter, if shifted backward, usually bigger
  append(&msg, std::to_string(track->trackMetrics->track_size));  //in bytes
  append(
      &msg,
      std::to_string(
          track->trackMetrics
              ->totalAmountOfPlayTime));  //total millis from start to end (sometimes a little longer than millis_played) pause has no influence
  append(&msg,
         std::to_string(
             track->trackMetrics->totalAmountPlayed));  //total millis played
  append(&msg,
         std::to_string(track->trackInfo.duration));  //track duration in millis
  append(
      &msg,
      std::to_string(
          track->trackMetrics
              ->trackHeaderTime));  //total time of decrypting? often 3 or 4 when nothing played -1
  append(&msg, "0");  // fadeoverlapp? usually 0, but fading is set on
  append(
      &msg,
      "0");  // librespot says, could bee that the first value shows if it is the first played song, usually 0
  append(&msg, "0");  //?? usually 0
  append(&msg, "0");  //?? usually 0
  append(&msg, std::to_string(track->trackMetrics->skipped_backward
                                  .count));  //total times skipped backward
  append(&msg,
         std::to_string(track->trackMetrics->skipped_backward
                            .amount));  //total amount of time skipped backward
  append(&msg, std::to_string(track->trackMetrics->skipped_forward
                                  .count));  //total times skipped forward
  append(&msg,
         std::to_string(track->trackMetrics->skipped_forward
                            .amount));  //total amount of time skipped forward
  append(
      &msg,
      "15");  //randomNumber; biggest value so far 260, usually biggert than 1 //spotify says play latencie
  append(&msg, "-1");  // usually -1, if paused positive, probablly in seconds
  append(&msg, "context");
  append(&msg, std::to_string(
                   track->trackMetrics
                       ->audioKeyTime));  //time to get the audiokey? usually -1
  append(&msg, "0");                      // usually 0
  append(&msg, "1");  // @librespot audioKey preloaded? usually positive (1)
  append(&msg, "0");  //?? usually 0
  append(
      &msg,
      "0");  //?? usually bigger than 0, if startup(not playing) or takenover "0"
  append(
      &msg,
      "1");  // usually 1 , if taken over 0, if player took over not from the start 0
  append(&msg,
         std::to_string(
             track->trackMetrics->longestInterval));  //length longest interval
  append(
      &msg,
      std::to_string(
          track->trackMetrics
              ->totalMsOfPlayedTrack));  //total time since start total millis of song played
  append(&msg, "0");                     //?? usually 0
  append(
      &msg,
      "320000");  //CONFIG_CSPOT_AUDIO_FORMAT ? CONFIG_CSPOT_AUDIO_FORMAT == 2 ? "320000" : "160000" : "96000"); // bitrate
  append(&msg, context_uri);
  append(&msg, "vorbis");
  append(
      &msg,
      track->trackInfo
          .trackId);  // sometimes, when commands come from outside, or else, this value is something else and the following one is the gid
  append(&msg, "");
  append(&msg, "0");  //?? usually 0
  append(&msg,
         std::to_string(track->trackMetrics
                            ->timestamp));  // unix timestamp when track started
  append(&msg, "0");                        //?? usually 0
  append(&msg, track->ref.context == "radio" ? "autoplay" : "context");
  append(
      &msg,
      (end_source == "playlist" || end_source == "your_library")
          ? "your_library"
          : "search");  //std::to_string(this->ctx->referrerIdentifier));//your_library:lybrary_header / autoplay:afterplaylist / search:found in search / find /home:found in home
  append(
      &msg,
      "xpui_2024-05-31_1717155884878_");  //xpui_2024-05-31_1717155884878_ xpui_ + date of version(update) + _ + unix_timestamp probably of update //empty if autoplay
  append(&msg, "com.spotify");
  append(
      &msg,
      "none");  //"transition" : gapless if song ended beforehand inside player
  append(&msg, "none");
  append(
      &msg,
      "local");  // @librespot lastCommandSentByDeviceId , usually"local", if orderd from outside last command ident last comment ident from frame.state
  append(&msg, "na");
  append(&msg, "none");
  append(
      &msg,
      "");  // 3067b220-9721-4489-93b4-118dd3a59b7b //page-instance-id // changing , if radio / autoplay empty
  append(
      &msg,
      "");  //    054dfc5a-0108-4971-b8de-1fd95070e416 //interaction_id // stays still , if radio / autoplay capped
  append(&msg, "");
  append(&msg, "5003900000000146");
  append(&msg, "");
  append(
      &msg,
      track->ref.context == "radio"
          ? correlation_id
          : "");  //ssp~061a6236e23e1a9847bd9b6ad4cd942eac8d //allways the same , only when radio? / autoplay
  append(&msg, "");
  append(&msg, "0");  //?? usually 0
  append(&msg, "0");  //?? usually 0

  if (end_reason == END_PLAY)
    start_reason = nested_reason;
  else
    start_reason = end_reason;
  start_source = end_source;

#ifndef CONFIG_HIDDEN
  auto responseLambda = [=](MercurySession::Response& res) {
  };

  auto parts = MercurySession::DataParts({msg});
  // Execute the request
  ctx->session->execute(MercurySession::RequestType::SEND, requestUrl,
                        responseLambda, parts);
#endif
  return msg;
}