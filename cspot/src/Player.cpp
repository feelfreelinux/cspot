#include "Player.h"

Player::Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink)
{
    this->audioSink = audioSink;
    this->manager = manager;
    // this->audioSink = std::make_shared<I2SAudioSink>();
    
}
void Player::pause()
{
    this->currentTrack->audioStream->isPaused = true;
}

void Player::play()
{
    this->currentTrack->audioStream->isPaused = false;
}

void Player::seekMs(size_t positionMs) {
    printf("----- Tryin to seek %d\n", positionMs);
    this->currentTrack->audioStream->seekMs(positionMs);
}

void Player::handleLoad(TrackRef *track, std::function<void()> &trackLoadedCallback)
{
    if (currentTrack != nullptr)
    {
        if (currentTrack->audioStream->isRunning) {
            currentTrack->audioStream->isRunning = false;
            while (!currentTrack->audioStream->finished) {
                        //vTaskDelay(100);
            }
        }

        delete currentTrack;
    }

    auto gid = std::vector<uint8_t>(track->gid->bytes, track->gid->bytes + 16);
    currentTrack = new SpotifyTrack(this->manager, gid);
    currentTrack->loadedTrackCallback = [=]() {
        trackLoadedCallback();
        currentTrack->audioStream->streamFinishedCallback = this->endOfFileCallback;
        currentTrack->audioStream->audioSink = audioSink;
        currentTrack->audioStream->startTask();
    };
}
