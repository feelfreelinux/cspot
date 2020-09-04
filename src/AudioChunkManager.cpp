#include "AudioChunkManager.h"

AudioChunkManager::AudioChunkManager()
{
    this->chunks = std::vector<std::shared_ptr<AudioChunk>>();
}

std::shared_ptr<AudioChunk> AudioChunkManager::registerNewChunk(uint16_t seqId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos)
{
    auto chunk = std::make_shared<AudioChunk>(seqId, audioKey, startPos * 4, endPos * 4);
    this->chunks.push_back(chunk);

    return chunk;
}

void AudioChunkManager::handleChunkData(std::vector<uint8_t> data)
{
    uint16_t seqId = ntohs(extract<uint16_t>(data, 0));

    for (auto const &chunk : this->chunks)
    {
        // Found the right chunk
        if (chunk->seqId == seqId)
        {
            switch (data.size())
            {
            case DATA_SIZE_HEADER:
            {
                auto headerSize = ntohs(extract<uint16_t>(data, 2));
                // Got file size!
                chunk->headerFileSize = ntohl(extract<uint32_t>(data, 5)) * 4;
                printf("ID: %d: Total size %d\n", seqId, chunk->headerFileSize);
                break;
            }
            case DATA_SIZE_FOOTER:
                if (chunk->endPosition > chunk->headerFileSize) {
                    chunk->endPosition = chunk->headerFileSize;
                }
                chunk->decrypt();
                printf("ID: %d: Finished!\n", seqId);
                break;

            default:
                printf("ID: %d: Got data chunk!\n", seqId);
                // 2 first bytes are size so we skip it
                auto actualData = std::vector<uint8_t>(data.begin() + 2, data.end());
                chunk->appendData(actualData);
                break;
            }
        }
    }
}
