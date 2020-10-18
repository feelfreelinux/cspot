#ifndef SPOTIFYTRACK_H
#define SPOTIFYTRACK_H

#include <vector>
#include <iostream>
#include "MercuryManager.h"
#include "PBUtils.h"
#include "metadata.pb.h"
#include "Utils.h"
#include "MercuryResponse.h"
#include <fstream>
#include <functional>
#include "ChunkedAudioStream.h"


class SpotifyTrack
{
private:
    std::shared_ptr<MercuryManager> manager;
    void trackInformationCallback(std::unique_ptr<MercuryResponse> response);

    std::vector<uint8_t> fileId;
    std::vector<uint8_t> currentChunkData;
    std::vector<uint8_t> currentChunkHeader;
public:
    SpotifyTrack(std::shared_ptr<MercuryManager> manager, std::vector<uint8_t> &gid);
    ~SpotifyTrack();
    uint64_t reqSeqNum = -1;
    std::function<void()> loadedTrackCallback;
    std::unique_ptr<ChunkedAudioStream> audioStream;
    audioKeyCallback audioKeyLambda;
    mercuryCallback responseLambda;
};


#endif