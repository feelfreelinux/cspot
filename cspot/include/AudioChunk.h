#ifndef AUDIOCHUNK_H
#define AUDIOCHUNK_H
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include "pthread.h"
#include "WrappedSemaphore.h"
#include "Crypto.h"

class AudioChunk {
private:
    std::vector<uint8_t> getIVSum(uint32_t num);

public:
    std::unique_ptr<Crypto> crypto;
    std::vector<uint8_t> decryptedData;
    std::vector<uint8_t> audioKey;
    bool keepInMemory = false;
    pthread_mutex_t loadingMutex;
    uint32_t startPosition;
    uint32_t endPosition;
    uint16_t seqId;

    size_t headerFileSize = -1;
    bool isLoaded = false;
    std::unique_ptr<WrappedSemaphore> isLoadedSemaphore;
    std::unique_ptr<WrappedSemaphore> isHeaderFileSizeLoadedSemaphore;



    AudioChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPosition, uint32_t predictedEndPosition);
    ~AudioChunk();
    void appendData(std::vector<uint8_t> &data);
    void decrypt();
};

#endif