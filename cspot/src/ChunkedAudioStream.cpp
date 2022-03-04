#include "ChunkedAudioStream.h"
#include "Logger.h"
#include "BellUtils.h"

static size_t vorbisReadCb(void *ptr, size_t size, size_t nmemb, ChunkedAudioStream *self)
{
    size_t readSize = 0;
    while (readSize < nmemb * size && self->byteStream->position() < self->byteStream->size()) {
        readSize += self->byteStream->read((uint8_t *) ptr + readSize, (size * nmemb) - readSize);
    }
    return readSize;
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
    return static_cast<long>(self->byteStream->position());
}

ChunkedAudioStream::~ChunkedAudioStream()
{
}

ChunkedAudioStream::ChunkedAudioStream(std::vector<uint8_t> fileId, std::vector<uint8_t> audioKey, uint32_t duration, std::shared_ptr<MercuryManager> manager, uint32_t startPositionMs, bool isPaused)
{
    this->duration = duration;
    this->startPositionMs = startPositionMs;
    this->isPaused = isPaused;

//    auto beginChunk = manager->fetchAudioChunk(fileId, audioKey, 0, 0x4000);
//    beginChunk->keepInMemory = true;
//    while(beginChunk->isHeaderFileSizeLoadedSemaphore->twait() != 0);
//    this->fileSize = beginChunk->headerFileSize;
//    chunks.push_back(beginChunk);
//
//    // File size is required for this packet to be downloaded
//    this->fetchTraillingPacket();

    this->byteStream = std::make_shared<ChunkedByteStream>(manager);
    this->byteStream->setFileInfo(fileId, audioKey);
    this->byteStream->fetchFileInformation();
    vorbisFile = { };
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
    byteStream->setEnableLoadAhead(false);
    this->seekMutex.lock();
    ov_time_seek(&vorbisFile, positionMs);
    this->seekMutex.unlock();
    byteStream->setEnableLoadAhead(true);

    CSPOT_LOG(debug, "--- Finished seeking!");
}

void ChunkedAudioStream::startPlaybackLoop(uint8_t *pcmOut, size_t pcmOut_len)
{

    isRunning = true;

    byteStream->setEnableLoadAhead(false);
    int32_t r = ov_open_callbacks(this, &vorbisFile, NULL, 0, vorbisCallbacks);
    CSPOT_LOG(debug, "--- Loaded file");
    if (this->startPositionMs != 0)
    {
         ov_time_seek(&vorbisFile, startPositionMs);
    }

    bool eof = false;
    byteStream->setEnableLoadAhead(true);

    while (!eof && isRunning)
    {
        if (!isPaused)
        {

            this->seekMutex.lock();
            long ret = ov_read(&vorbisFile, (char *)&pcmOut[0], pcmOut_len, &currentSection);
            this->seekMutex.unlock();
            if (ret == 0)
            {
                CSPOT_LOG(info, "EOL");
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
                pcmCallback(pcmOut, ret);
                // audioSink->feedPCMFrames(data);
            }
        }
        else
        {
            BELL_SLEEP_MS(100);
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
//
//void ChunkedAudioStream::fetchTraillingPacket()
//{
//    auto startPosition = (this->fileSize / 4) - 0x1000;
//
//    // AES block size is 16, so the index must be divisible by it
//    while ((startPosition * 4) % 16 != 0)
//        startPosition++; // ik, ugly lol
//
//    auto endChunk = manager->fetchAudioChunk(fileId, audioKey, startPosition, fileSize / 4);
//    endChunk->keepInMemory = true;
//
//    chunks.push_back(endChunk);
//    while (endChunk->isLoadedSemaphore->twait() != 0);
//}

void ChunkedAudioStream::seek(size_t dpos, Whence whence)
{
    auto seekPos = 0;
    switch (whence)
    {
    case Whence::START:
        seekPos = dpos;
        break;
    case Whence::CURRENT:
        seekPos = byteStream->position() + dpos;
        break;
    case Whence::END:
        seekPos = byteStream->size() + dpos;
        break;
    }
    byteStream->seek(seekPos);
}
