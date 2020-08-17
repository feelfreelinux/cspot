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
#include "aes.h"

class SpotifyTrack
{
private:
    std::shared_ptr<MercuryManager> manager;
    void trackInformationCallback(std::unique_ptr<MercuryResponse> response);
    void processAudioChunk(bool status, std::vector<uint8_t> data);

    std::vector<uint8_t> audioKey;
    uint16_t currentChunk = 0;
    std::vector<uint8_t> fileId;
    std::vector<uint8_t> currentChunkData;
    std::vector<uint8_t> currentChunkHeader;
    std::vector<uint8_t> trackId;
    std::ofstream oggFile;
    struct AES_ctx ctx;
public:
    SpotifyTrack(std::shared_ptr<MercuryManager> manager);
};


#endif