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
    size_t pos = 167;
    pthread_mutex_t writeMutex;
    bool loadingChunks;    
    bool finished = false;
    struct AES_ctx ctx;
public:
    SpotifyTrack(std::shared_ptr<MercuryManager> manager, std::string UserField_CALLBACK);
    std::function<void()> loadedTrackCallback;
    std::vector<std::vector<uint8_t>> chunkBuffer;
    std::vector<uint8_t> read(size_t bytes);
    size_t seek(size_t pos);
    bool firstChunkLoaded = false;
};


#endif