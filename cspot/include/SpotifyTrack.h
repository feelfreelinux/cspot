#ifndef SPOTIFYTRACK_H
#define SPOTIFYTRACK_H

#include <memory>
#include <vector>
#include <iostream>
#include "MercuryManager.h"
#include "Utils.h"
#include "MercuryResponse.h"
#include <fstream>
#include "Crypto.h"
#include <functional>
#include "ChunkedAudioStream.h"
#include "TrackReference.h"
#include "NanoPBHelper.h"
#include "protobuf/metadata.pb.h"
#include <cassert>
#include <atomic>

struct TrackInfo {
    std::string name;
    std::string album;
    std::string artist;
    std::string imageUrl;
    int duration;
};

typedef std::function<void(TrackInfo&)> trackChangedCallback;

class SpotifyTrack
{
private:
    std::shared_ptr<MercuryManager> manager;
    void trackInformationCallback(std::unique_ptr<MercuryResponse> response, uint32_t position_ms, bool isPaused);
    void episodeInformationCallback(std::unique_ptr<MercuryResponse> response, uint32_t position_ms, bool isPaused);
    void requestAudioKey(std::vector<uint8_t> fileId, std::vector<uint8_t> trackId, int32_t trackDuration, uint32_t position_ms, bool isPaused);
    bool countryListContains(char *countryList, char *country);
    bool canPlayTrack(int altIndex);
    Track trackInfo;
    Episode episodeInfo;

    std::vector<uint8_t> fileId;
    std::vector<uint8_t> currentChunkData;
    std::vector<uint8_t> currentChunkHeader;
public:
    SpotifyTrack(std::shared_ptr<MercuryManager> manager, std::shared_ptr<TrackReference> trackRef, uint32_t position_ms, bool isPaused);
    ~SpotifyTrack();
    uint64_t reqSeqNum = -1;
    std::atomic<bool> loaded = false;
    std::function<void()> loadedTrackCallback;
    std::unique_ptr<ChunkedAudioStream> audioStream;
    trackChangedCallback trackInfoReceived;
    audioKeyCallback audioKeyLambda;
    mercuryCallback responseLambda;
};


#endif
