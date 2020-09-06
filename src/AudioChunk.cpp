#include "AudioChunk.h"

std::vector<uint8_t> audioAESIV({0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93});

AudioChunk::AudioChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPosition, uint32_t predictedEndPosition)
{
    this->seqId = seqId;
    this->audioKey = audioKey;
    this->startPosition = startPosition;
    this->endPosition = predictedEndPosition;
    this->decryptedData = std::vector<uint8_t>();
}

void AudioChunk::appendData(std::vector<uint8_t> &data) {
    this->decryptedData.insert(this->decryptedData.end(), data.begin(), data.end());
}

void AudioChunk::decrypt() {
    // calculate the IV for right position
    auto calculatedIV = this->getIVSum(startPosition / 16);
    AES_init_ctx_iv(&this->ctx, &this->audioKey[0], &calculatedIV[0]);
    AES_CTR_xcrypt_buffer(&this->ctx, &this->decryptedData[0], this->decryptedData.size());

    this->startPosition = this->endPosition - this->decryptedData.size();
    this->isLoaded = true;
}

// Basically just big num addition
std::vector<uint8_t> AudioChunk::getIVSum(uint32_t num)
{
    auto digits = audioAESIV;
    std::reverse(digits.begin(), digits.end());
    auto otherNum = std::vector<uint8_t>();
    while (num)
    {
        otherNum.push_back(num % 256);
        num /= 256;
    }
    int sum = 0;
    auto i = digits.begin();
    auto j = otherNum.begin();
    bool in = true;
    bool jn = true;
    while ((in &= i != digits.end()) | (jn &= j != otherNum.end()) || sum)
    {
        if (jn)
            sum += *j++;
        if (in)
        {
            sum += *i;
            *i++ = sum % 256;
        }
        else
        {
            digits.push_back(sum % 256);
        }
        sum /= 256; // carry to next digit
    };
    std::reverse(digits.begin(), digits.end());

    return digits;
}
