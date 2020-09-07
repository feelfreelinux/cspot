#include "ChunkedAudioStream.h"

static size_t vorbisReadCb(void *ptr, size_t size, size_t nmemb, ChunkedAudioStream *self)
{
    auto data = self->read(nmemb);
    self->readBeforeSeek += data.size();
    std::copy(data.begin(), data.end(), (char *)ptr);
    return data.size();
}

static int vorbisSeekCb(ChunkedAudioStream *self, int64_t offset, int whence)
{
    if (whence == 0)
    {
        offset += SPOTIFY_HEADER_SIZE;
    }

    static constexpr std::array<Whence, 3> seekDirections{
        Whence::START, Whence::CURRENT, Whence::END};

    self->seek(offset, seekDirections.at(static_cast<size_t>(whence)));
    return 0;
}

static long vorbisTellCb(ChunkedAudioStream *self)
{
    return static_cast<long>(self->pos);
}

ChunkedAudioStream::ChunkedAudioStream(std::vector<uint8_t> fileId, std::vector<uint8_t> audioKey, uint32_t duration, std::shared_ptr<MercuryManager> manager, uint32_t startPositionMs)
{
    this->audioSink = audioSink;
    this->audioKey = audioKey;
    this->duration = duration;
    this->manager = manager;
    this->fileId = fileId;
    this->startPositionMs = startPositionMs;
    pthread_mutex_init(&seekMutex, NULL);

    readBeforeSeek = getCurrentTimestamp();

    auto beginChunk = manager->fetchAudioChunk(fileId, audioKey, 0, 0x4000);
    beginChunk->keepInMemory = true;

    while (beginChunk->headerFileSize == -1);
    this->fileSize = beginChunk->headerFileSize;
    chunks.push_back(beginChunk);

    // File size is required for this packet to be downloaded
    this->fetchTraillingPacket();

    vorbisFile = {0};
    vorbisCallbacks =
        {
            (decltype(ov_callbacks::read_func)) & vorbisReadCb,
            (decltype(ov_callbacks::seek_func)) & vorbisSeekCb,
            nullptr, // Close
            (decltype(ov_callbacks::tell_func)) & vorbisTellCb,
        };
}

void ChunkedAudioStream::seekMs(uint32_t positionMs)
{
    pthread_mutex_lock(&seekMutex);
    ov_time_seek(&vorbisFile, positionMs);
    pthread_mutex_unlock(&seekMutex);
    printf("--- Finished seeking!");
}

void ChunkedAudioStream::runTask()
{
    // while (!loadedMeta);
    isRunning = true;

    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
    printf("--- Loaded file\n");
    if (this->startPositionMs != 0)
    {
        ov_time_seek(&vorbisFile, startPositionMs);
    }

    bool eof = false;
    while (!eof && isRunning)
    {
        if (!isPaused)
        {
            std::vector<uint8_t> pcmOut(4096);
            pthread_mutex_lock(&seekMutex);

            long ret = ov_read(&vorbisFile, (char *)&pcmOut[0], 4096, &currentSection);
            pthread_mutex_unlock(&seekMutex);

            if (ret == 0)
            {
                // and done :)
                eof = true;
            }
            else if (ret < 0)
            {
                // Error in the stream
            }
            else
            {
                // Write the actual data
                auto data = std::vector<uint8_t>(pcmOut.begin(), pcmOut.begin() + ret);
                audioSink->feedPCMFrames(data);
            }
        }
    }

    printf("Track finished \n");

    finished = true;

    if (eof)
    {
        this->streamFinishedCallback();
    }
}

void ChunkedAudioStream::fetchTraillingPacket()
{
    auto startPosition = (this->fileSize / 4) - 0x1000;

    // AES block size is 16, so the index must be divisible by it
    while ((startPosition * 4) % 16 != 0)
        startPosition++; // ik, ugly lol

    auto endChunk = manager->fetchAudioChunk(fileId, audioKey, startPosition, fileSize / 4);
    endChunk->keepInMemory = true;

    chunks.push_back(endChunk);
    while (endChunk->isLoaded == false);

    loadedMeta = true;
}

std::vector<uint8_t> ChunkedAudioStream::read(size_t bytes)
{
    auto toRead = bytes;
    auto res = std::vector<uint8_t>();
    // printf("Dupa, trying to read %d\n", bytes);
    // printf("Pos, trying to read %d\n", pos);

    while (res.size() < bytes)
    {
        int16_t chunkIndex = this->pos / AUDIO_CHUNK_SIZE;
        int32_t offset = this->pos % AUDIO_CHUNK_SIZE;

        if (pos >= fileSize)
        {

            printf("EOL!\n");
            return res;
        }

        auto chunk = findChunkForPosition(pos);

        if (chunk != nullptr)
        {
            auto offset = pos - chunk->startPosition;
            if (chunk->isLoaded)
            {
                if (chunk->decryptedData.size() - offset >= toRead)
                {
                    res.insert(res.end(), chunk->decryptedData.begin() + offset, chunk->decryptedData.begin() + offset + toRead);
                    this->pos += toRead;
                }
                else
                {
                    res.insert(res.end(), chunk->decryptedData.begin() + offset, chunk->decryptedData.end());
                    this->pos += chunk->decryptedData.size() - offset;
                    toRead -= chunk->decryptedData.size() - offset;
                }
            }
        }
        else
        {
            this->requestChunk(chunkIndex);
        }

        auto requestedOffset = 0;

        while (requestedOffset < BUFFER_SIZE)
        {
            auto chunk = findChunkForPosition(pos + requestedOffset);

            if (chunk != nullptr)
            {
                requestedOffset = chunk->endPosition - pos;
            }

            // if (pos + requestedOffset >= fileSize) {
            //     break;
            // }

            else
            {
                auto chunkReq = manager->fetchAudioChunk(fileId, audioKey, (pos + requestedOffset) / 4, (pos + requestedOffset + AUDIO_CHUNK_SIZE) / 4);
                printf("Chunk req end pos %d\n", chunkReq->endPosition);
                this->chunks.push_back(chunkReq);
            }
        }
        auto position = pos;

        // Erase all chunks not close to current position
        chunks.erase(std::remove_if(
                         chunks.begin(), chunks.end(),
                         [&position](const std::shared_ptr<AudioChunk> &chunk) {
                             if (chunk->keepInMemory)
                             {
                                 return false;
                             }

                             if (chunk->endPosition < position || chunk->startPosition > position + BUFFER_SIZE)
                             {
                                 return true;
                             }

                             return false;
                         }),
                     chunks.end());
    }
    return res;
}

std::shared_ptr<AudioChunk> ChunkedAudioStream::findChunkForPosition(size_t position)
{
    for (int i = 0; i < this->chunks.size(); i++)
    {
        auto chunk = this->chunks[i];
        if (chunk->startPosition <= position && chunk->endPosition > position)
        {
            return chunk;
        }
    }

    return nullptr;
}

void ChunkedAudioStream::seek(size_t dpos, Whence whence)
{
    switch (whence)
    {
    case Whence::START:
        this->pos = dpos;
        break;
    case Whence::CURRENT:
        this->pos += dpos;
        break;
    case Whence::END:
        this->pos = fileSize + dpos;
        break;
    }

    auto currentChunk = this->pos / AUDIO_CHUNK_SIZE;
    printf("Change in current chunk %d\n", currentChunk);
}

void ChunkedAudioStream::requestChunk(size_t chunkIndex)
{

    printf("Chunk Req %d\n", chunkIndex);
    this->chunks.push_back(manager->fetchAudioChunk(fileId, audioKey, chunkIndex));
}