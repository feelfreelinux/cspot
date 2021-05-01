#include "ChunkedAudioStream.h"
#include "Logger.h"

static size_t vorbisReadCb(void *ptr, size_t size, size_t nmemb, ChunkedAudioStream *self)
{
    auto data = self->read(nmemb);
    std::copy(data.begin(), data.end(), (char *)ptr);
    return data.size();
}
static int vorbisCloseCb(ChunkedAudioStream *self)
{
    return 0;
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

ChunkedAudioStream::~ChunkedAudioStream()
{
}

ChunkedAudioStream::ChunkedAudioStream(std::vector<uint8_t> fileId, std::vector<uint8_t> audioKey, uint32_t duration, std::shared_ptr<MercuryManager> manager, uint32_t startPositionMs)
{
    this->audioSink = audioSink;
    this->audioKey = audioKey;
    this->duration = duration;
    this->manager = manager;
    this->fileId = fileId;
    this->startPositionMs = startPositionMs;

    auto beginChunk = manager->fetchAudioChunk(fileId, audioKey, 0, 0x4000);
    beginChunk->keepInMemory = true;
    beginChunk->isHeaderFileSizeLoadedSemaphore->wait();
    this->fileSize = beginChunk->headerFileSize;
    chunks.push_back(beginChunk);

    // File size is required for this packet to be downloaded
    this->fetchTraillingPacket();

    vorbisFile = {0};
    vorbisCallbacks =
        {
            (decltype(ov_callbacks::read_func)) & vorbisReadCb,
            (decltype(ov_callbacks::seek_func)) & vorbisSeekCb,
            (decltype(ov_callbacks::close_func)) & vorbisCloseCb,
            (decltype(ov_callbacks::tell_func)) & vorbisTellCb,
        };
}

void ChunkedAudioStream::seekMs(uint32_t positionMs)
{

    this->seekMutex.lock();
    loadingMeta = true;
    ov_time_seek(&vorbisFile, positionMs);
    loadingMeta = false;
    this->seekMutex.unlock();

    CSPOT_LOG(debug, "--- Finished seeking!");
}

void ChunkedAudioStream::startPlaybackLoop()
{

    loadingMeta = true;
    isRunning = true;

    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
    CSPOT_LOG(debug, "--- Loaded file");
    if (this->startPositionMs != 0)
    {
        ov_time_seek(&vorbisFile, startPositionMs);
    }
    else
    {
        this->requestChunk(0);
    }

    loadingMeta = false;
    bool eof = false;
    while (!eof && isRunning)
    {
        if (!isPaused)
        {
            std::vector<uint8_t> pcmOut(4096 / 4);

            this->seekMutex.lock();
            long ret = ov_read(&vorbisFile, (char *)&pcmOut[0], 4096 / 4, &currentSection);
            this->seekMutex.unlock();
            if (ret == 0)
            {
                // and done :)
                eof = true;
            }
            else if (ret < 0)
            {
                CSPOT_LOG(error, "An error has occured in the stream");

                // Error in the stream
            }
            else
            {
                // Write the actual data
                auto data = std::vector<uint8_t>(pcmOut.begin(), pcmOut.begin() + ret);
                pcmCallback(data);
                // audioSink->feedPCMFrames(data);
            }
        }
        else
        {
            usleep(100 * 1000);
        }
    }

    ov_clear(&vorbisFile);
    vorbisCallbacks = {};
    CSPOT_LOG(debug, "Track finished");
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
    endChunk->isLoadedSemaphore->wait();
}

std::vector<uint8_t> ChunkedAudioStream::read(size_t bytes)
{
    auto toRead = bytes;
    auto res = std::vector<uint8_t>();
READ:
    while (res.size() < bytes)
    {
        auto position = pos;

        // Erase all chunks not close to current position
        chunks.erase(std::remove_if(
                         chunks.begin(), chunks.end(),
                         [&position](const std::shared_ptr<AudioChunk> &chunk) {
                             if (chunk->keepInMemory)
                             {
                                 return false;
                             }

                             if (chunk->isFailed)
                             {
                                 return true;
                             }

                             if (chunk->endPosition < position || chunk->startPosition > position + BUFFER_SIZE)
                             {
                                 return true;
                             }

                             return false;
                         }),
                     chunks.end());

        int16_t chunkIndex = this->pos / AUDIO_CHUNK_SIZE;
        int32_t offset = this->pos % AUDIO_CHUNK_SIZE;

        if (pos >= fileSize)
        {

            CSPOT_LOG(debug, "EOL!");
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
            else
            {
                chunk->isLoadedSemaphore->wait();
                if (chunk->isFailed)
                {
                    this->requestChunk(chunkIndex)->isLoadedSemaphore->wait();
                    goto READ;
                }
            }
        }
        else
        {
            CSPOT_LOG(debug, "Actual request %d", chunkIndex);
            this->requestChunk(chunkIndex);
        }
    }

    if (!loadingMeta)
    {

        auto requestedOffset = 0;

        while (requestedOffset < BUFFER_SIZE)
        {
            auto chunk = findChunkForPosition(pos + requestedOffset);

            if (chunk != nullptr)
            {
                requestedOffset = chunk->endPosition - pos;

                // Don not buffer over EOL - unnecessary "failed chunks"
                if ((pos + requestedOffset) >= fileSize)
                {
                    break;
                }
            }

            else
            {
                auto chunkReq = manager->fetchAudioChunk(fileId, audioKey, (pos + requestedOffset) / 4, (pos + requestedOffset + AUDIO_CHUNK_SIZE) / 4);
                CSPOT_LOG(debug, "Chunk req end pos %d", chunkReq->endPosition);
                this->chunks.push_back(chunkReq);
            }
        }
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

    if (findChunkForPosition(this->pos) == nullptr)
    {
        // Seeking might look back - therefore we preload some past data
        auto startPosition = (this->pos / 4) - (AUDIO_CHUNK_SIZE / 4);

        // AES block size is 16, so the index must be divisible by it
        while ((startPosition * 4) % 16 != 0)
            startPosition++; // ik, ugly lol

        this->chunks.push_back(manager->fetchAudioChunk(fileId, audioKey, startPosition, startPosition + (AUDIO_CHUNK_SIZE / 4)));
    }
    CSPOT_LOG(debug, "Change in current chunk %d", currentChunk);
}

std::shared_ptr<AudioChunk> ChunkedAudioStream::requestChunk(size_t chunkIndex)
{

    CSPOT_LOG(debug, "Chunk Req %d", chunkIndex);
    auto chunk = manager->fetchAudioChunk(fileId, audioKey, chunkIndex);
    this->chunks.push_back(chunk);
    return chunk;
}
