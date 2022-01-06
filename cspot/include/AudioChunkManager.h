#ifndef AUDIOCHUNKMANAGER_H
#define AUDIOCHUNKMANAGER_H

#include <memory>
#include <atomic>
#include <algorithm>
#include <mutex>
#include "Utils.h"
#include "AudioChunk.h"
#include "Queue.h"
#include "Task.h"

#define DATA_SIZE_HEADER 24
#define DATA_SIZE_FOOTER 2

class AudioChunkManager : public bell::Task {
    std::vector<std::shared_ptr<AudioChunk>> chunks;
    bell::Queue<std::pair<std::vector<uint8_t>, bool>> audioChunkDataQueue;
    void runTask();
public:
    AudioChunkManager();
    std::atomic<bool> isRunning = false;
    std::mutex runningMutex;
    std::mutex chunkMutex;
    /**
     * @brief Registers a new audio chunk request.
     * 
     * Registering an audiochunk will trigger a request to spotify servers.
     * All the incoming data will be redirected to this given audiochunk.
     * 
     * @param seqId sequence identifier of given audio chunk.
     * @param audioKey audio key of given file, used for decryption.
     * @param startPos start position of audio chunk
     * @param endPos end position of audio chunk. end - pos % 4 must be 0.
     * @return std::shared_ptr<AudioChunk> registered audio chunk. Does not contain the data yet.
     */
    std::shared_ptr<AudioChunk> registerNewChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos);

    /**
     * @brief Pushes binary data from spotify's servers containing audio chunks.
     * 
     * This method pushes received data to a queue that is then received by manager's thread.
     * That thread parses the data and passes it to a matching audio chunk.
     * 
     * @param data binary data received from spotify's servers
     * @param failed whenever given chunk request failed
     */
    void handleChunkData(std::vector<uint8_t>& data, bool failed = false);

    /**
     * @brief Fails all requested chunks, used for reconnection.
     */
    void failAllChunks();

    void close();
};

#endif
