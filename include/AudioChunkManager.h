#ifndef AUDIOCHUNKMANAGER_H
#define AUDIOCHUNKMANAGER_H

#include <memory>
#include "Utils.h"
#include "AudioChunk.h"
#define DATA_SIZE_HEADER 24
#define DATA_SIZE_FOOTER 2

class AudioChunkManager {
    std::vector<std::shared_ptr<AudioChunk>> chunks;
public:
    AudioChunkManager();
    std::shared_ptr<AudioChunk> registerNewChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos);
    void handleChunkData(std::vector<uint8_t> data);
};

#endif