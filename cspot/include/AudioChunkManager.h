#ifndef AUDIOCHUNKMANAGER_H
#define AUDIOCHUNKMANAGER_H

#include <memory>
#include <algorithm>
#include "Utils.h"
#include "AudioChunk.h"
#include "Queue.h"
#include "Task.h"

#define DATA_SIZE_HEADER 24
#define DATA_SIZE_FOOTER 2

class AudioChunkManager : public Task {
    std::vector<std::shared_ptr<AudioChunk>> chunks;
    void runTask();
public:
    AudioChunkManager();
    Queue<std::pair<std::vector<uint8_t>, bool>> audioChunkDataQueue;
    std::shared_ptr<AudioChunk> registerNewChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos);
    void handleChunkData(std::vector<uint8_t>& data, bool failed = false);
    void failAllChunks();
};

#endif