#include "AudioChunkManager.h"
#include "BellUtils.h"
#include "Logger.h"

AudioChunkManager::AudioChunkManager()
    : bell::Task("AudioChunkManager", 4 * 1024, 5, 1) {
    this->chunks = std::vector<std::shared_ptr<AudioChunk>>();
    startTask();
}

std::shared_ptr<AudioChunk>
AudioChunkManager::registerNewChunk(uint16_t seqId,
                                    std::vector<uint8_t> &audioKey,
                                    uint32_t startPos, uint32_t endPos) {
    auto chunk =
        std::make_shared<AudioChunk>(seqId, audioKey, startPos * 4, endPos * 4);
    this->chunks.push_back(chunk);
    CSPOT_LOG(debug, "Chunk requested %d", seqId);

    return chunk;
}
void AudioChunkManager::handleChunkData(std::vector<uint8_t> &data,
                                        bool failed) {
    auto audioPair = std::pair(data, failed);
    audioChunkDataQueue.push(audioPair);
}

void AudioChunkManager::failAllChunks() {
    // Enumerate all the chunks and mark em all failed
    for (auto const &chunk : this->chunks) {
        if (!chunk->isLoaded) {
            chunk->isLoaded = true;
            chunk->isFailed = true;
            chunk->isHeaderFileSizeLoadedSemaphore->give();
            chunk->isLoadedSemaphore->give();
        }
    }
}

void AudioChunkManager::close() {
    this->isRunning = false;
    this->failAllChunks();
    this->audioChunkDataQueue.clear();
    std::scoped_lock lock(this->runningMutex);
}

void AudioChunkManager::runTask() {
    this->isRunning = true;
    std::scoped_lock lock(this->runningMutex);
    while (isRunning) {
        std::pair<std::vector<uint8_t>, bool> audioPair;
        if (this->audioChunkDataQueue.wtpop(audioPair, 100)) {
            auto data = audioPair.first;
            auto failed = audioPair.second;
            uint16_t seqId = ntohs(extract<uint16_t>(data, 0));

            // Erase all chunks that are not referenced elsewhere anymore
            chunks.erase(
                std::remove_if(chunks.begin(), chunks.end(),
                               [](const std::shared_ptr<AudioChunk> &chunk) {
                                   return chunk.use_count() == 1;
                               }),
                chunks.end());

            try {
                for (auto const &chunk : this->chunks) {
                    // Found the right chunk
                    if (chunk != nullptr && chunk->seqId == seqId) {
                        if (failed) {
                            // chunk->isFailed = true;
                            chunk->startPosition = 0;
                            chunk->endPosition = 0;
                            chunk->isHeaderFileSizeLoadedSemaphore->give();
                            chunk->isLoadedSemaphore->give();
                            break;
                        }

                        switch (data.size()) {
                        case DATA_SIZE_HEADER: {
                            CSPOT_LOG(debug, "ID: %d: header decrypt!", seqId);
                            auto headerSize = ntohs(extract<uint16_t>(data, 2));
                            // Got file size!
                            chunk->headerFileSize =
                                ntohl(extract<uint32_t>(data, 5)) * 4;
                            chunk->isHeaderFileSizeLoadedSemaphore->give();
                            break;
                        }
                        case DATA_SIZE_FOOTER:
                            if (chunk->endPosition > chunk->headerFileSize) {
                                chunk->endPosition = chunk->headerFileSize;
                            }
                            CSPOT_LOG(debug, "ID: %d: Starting decrypt!",
                                      seqId);
                            chunk->decrypt();
                            chunk->isLoadedSemaphore->give();
                            break;

                        default:
                            // printf("ID: %d: Got data chunk!\n", seqId);
                            // 2 first bytes are size so we skip it
                            // printf("(_)--- Free memory %d\n",
                            // esp_get_free_heap_size());
                            if (chunk == nullptr) {
                                return;
                            }
                            auto actualData = std::vector<uint8_t>(
                                data.begin() + 2, data.end());
                            chunk->appendData(actualData);
                            break;
                        }
                    }
                }

            } catch (...) {
            }
        } else {
        }
    }

    // Playback finished
}
