#ifndef CHUNKEDAUDIOSTREAM_H
#define CHUNKEDAUDIOSTREAM_H

#include <iostream>
#include <vector>
#include <fstream>
#include <array>
#include <unistd.h>
#include <atomic>
#include "ivorbisfile.h"
#include "MercuryManager.h"
#include "AudioSink.h"
#include "AudioChunk.h"
#include "platform/WrappedMutex.h"
#include "ChunkedByteStream.h"

#define SPOTIFY_HEADER_SIZE 167
#define BUFFER_SIZE 0x20000 * 1.5
typedef std::function<void(uint8_t *, size_t)> pcmDataCallback;

enum class Whence
{
    START,
    CURRENT,
    END
};

class ChunkedAudioStream
{
private:
    // Vorbis related
    OggVorbis_File vorbisFile;
    ov_callbacks vorbisCallbacks;
    int currentSection;

    // Audio data
    uint32_t duration;

    bool loadingChunks = false;
    uint16_t lastSequenceId = 0;

    std::shared_ptr<MercuryManager> manager;
    std::vector<uint8_t> fileId;
    uint32_t startPositionMs;

public:
    ChunkedAudioStream(std::vector<uint8_t> fileId, std::vector<uint8_t> audioKey, uint32_t duration, std::shared_ptr<MercuryManager> manager, uint32_t startPositionMs, bool isPaused);
    ~ChunkedAudioStream();
    std::shared_ptr<ChunkedByteStream> byteStream;

    std::function<void()> streamFinishedCallback;
    std::atomic<bool> isPaused = false;
    std::atomic<bool> isRunning = false;
    std::atomic<bool> finished = false;
    pcmDataCallback pcmCallback;
    std::shared_ptr<AudioSink> audioSink;
    WrappedMutex seekMutex;

    std::vector<uint8_t> read(size_t bytes);
    void seekMs(uint32_t positionMs);
    void seek(size_t pos, Whence whence);
    void startPlaybackLoop(uint8_t *pcmOut, size_t pcmOut_len);
};

#endif
