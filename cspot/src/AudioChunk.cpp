#include "AudioChunk.h"

std::vector<uint8_t> audioAESIV({0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93});

AudioChunk::AudioChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPosition, uint32_t predictedEndPosition)
{
    this->crypto = std::make_unique<Crypto>();
    this->seqId = seqId;
    this->audioKey = audioKey;
    this->startPosition = startPosition;
    this->endPosition = predictedEndPosition;
    this->decryptedData = std::vector<uint8_t>();
    this->isHeaderFileSizeLoadedSemaphore = std::make_unique<WrappedSemaphore>(5);
    this->isLoadedSemaphore = std::make_unique<WrappedSemaphore>(5);
}

AudioChunk::~AudioChunk()
{
}

void AudioChunk::appendData(const std::vector<uint8_t> &data)
{
    //if (this == nullptr) return;
    this->decryptedData.insert(this->decryptedData.end(), data.begin(), data.end());
}

void AudioChunk::readData(uint8_t *target, size_t offset, size_t nbytes) {
    auto readPos = offset + nbytes;
    auto modulo = (readPos % 16);
    auto ivReadPos = readPos;
    if (modulo != 0) {
        ivReadPos += (16 - modulo);
    }
    if (ivReadPos > decryptedCount) {
        // calculate the IV for right position
        auto calculatedIV = this->getIVSum((oldStartPos + decryptedCount) / 16);

        crypto->aesCTRXcrypt(this->audioKey, calculatedIV, decryptedData.data() + decryptedCount,  ivReadPos - decryptedCount);

        decryptedCount = ivReadPos;
    }
    memcpy(target, this->decryptedData.data() + offset, nbytes);

}

void AudioChunk::finalize()
{
    this->oldStartPos = this->startPosition;
    this->startPosition = this->endPosition - this->decryptedData.size();
    this->isLoaded = true;
}

// Basically just big num addition
std::vector<uint8_t> AudioChunk::getIVSum(uint32_t n)
{
    return bigNumAdd(audioAESIV, n);
}
