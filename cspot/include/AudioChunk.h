#ifndef AUDIOCHUNK_H
#define AUDIOCHUNK_H

#include <vector>
#include <string>
#include <algorithm>
#ifdef USE_MBEDTLS
#include <mbedtls/aes.h>
#else
#include <aes.h>
#endif

class AudioChunk {
private:
    std::vector<uint8_t> getIVSum(uint32_t num);
#ifdef USE_MBEDTLS
    mbedtls_aes_context ctx;
#else
    struct AES_ctx ctx;
#endif

public:
    std::vector<uint8_t> decryptedData;
    std::vector<uint8_t> audioKey;
    bool keepInMemory = false;
    uint32_t startPosition;
    uint32_t endPosition;
    uint16_t seqId;
    size_t headerFileSize = -1;
    bool isLoaded = false;

    AudioChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPosition, uint32_t predictedEndPosition);
    ~AudioChunk();
    void appendData(std::vector<uint8_t> &data);
    void decrypt();
};

#endif