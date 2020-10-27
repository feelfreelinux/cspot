#include "Player.h"

// #include <valgrind/memcheck.h>

Player::Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink)
{
    this->audioSink = audioSink;
    this->manager = manager;
    startTask();
}

void Player::pause()
{
    this->currentTrack->audioStream->isPaused = true;
}

void Player::play()
{
    this->currentTrack->audioStream->isPaused = false;
}

void Player::setVolume(uint16_t volume)
{
    this->volume = volume;

    // Pass volume event to the sink if volume is sink-handled
    if (!this->audioSink->softwareVolumeControl)
    {
        this->audioSink->volumeChanged(volume);
    }
}

void Player::seekMs(size_t positionMs)
{
    this->currentTrack->audioStream->seekMs(positionMs);
    // VALGRIND_DO_LEAK_CHECK;
}

void Player::feedPCM(std::vector<uint8_t> &data)
{
    // Simple digital volume control alg
    // @TODO actually extract it somewhere
    if (this->audioSink->softwareVolumeControl)
    {
        auto vol = 255 - volume;
        uint32_t value = (log10(255 / ((float)vol + 1)) * 105.54571334);
        if (value >= 254)
            value = 256;
        auto mult = value << 8; // *256
        int16_t *psample;
        uint32_t pmax;
        psample = (int16_t *)(data.data());
        for (int32_t i = 0; i < (data.size() / 2); i++)
        {
            int32_t temp = (int32_t)psample[i] * mult;
            psample[i] = (temp >> 16) & 0xFFFF;
        }
    }

    this->audioSink->feedPCMFrames(data);
}

void Player::runTask()
{
    while (true)
    {
        this->trackQueue.wpop(currentTrack);
        currentTrack->audioStream->startPlaybackLoop();
        currentTrack->loadedTrackCallback = nullptr;
        currentTrack->audioStream->streamFinishedCallback = nullptr;
        currentTrack->audioStream->audioSink = nullptr;
        currentTrack->audioStream->pcmCallback = nullptr;
    }
}

void Player::cancelCurrentTrack()
{
    if (currentTrack != nullptr)
    {
        if (currentTrack->audioStream != nullptr && currentTrack->audioStream->isRunning)
        {
            currentTrack->audioStream->isRunning = false;
        }
    }
}

void Player::handleLoad(std::shared_ptr<TrackReference> trackReference, std::function<void()> &trackLoadedCallback)
{
    std::lock_guard<std::mutex> guard(loadTrackMutex);
    cancelCurrentTrack();

    pcmDataCallback framesCallback = [=](std::vector<uint8_t> &frames) {
        this->feedPCM(frames);
    };

    auto loadedLambda = trackLoadedCallback;

    auto track = std::make_shared<SpotifyTrack>(this->manager, trackReference->gid);
    track->loadedTrackCallback = [this, track, framesCallback, loadedLambda]() {
        loadedLambda();
        track->audioStream->streamFinishedCallback = this->endOfFileCallback;
        track->audioStream->audioSink = this->audioSink;
        track->audioStream->pcmCallback = framesCallback;
        this->trackQueue.push(track);
    };
}
