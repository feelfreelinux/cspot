#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <math.h>
#include <functional>
#include <atomic>
#include <mutex>

#include "Utils.h"
#include "MercuryManager.h"
#include <ConfigJSON.h>
#include "TrackReference.h"
#include "Session.h"
#include "SpotifyTrack.h"
#include "AudioSink.h"
#include <mutex>
#include "BellTask.h"

class Player : public bell::Task {
private:
    std::shared_ptr<MercuryManager> manager;
    SpotifyTrack *currentTrack = nullptr;
    SpotifyTrack *nextTrack = nullptr;
    std::shared_ptr<AudioSink> audioSink;
    std::shared_ptr<ConfigJSON> config;
    std::mutex loadTrackMutex;
    WrappedMutex nextTrackMutex;
    WrappedMutex currentTrackMutex;
    void runTask();

public:
    Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink, std::shared_ptr<ConfigJSON> config);
    std::function<void()> endOfFileCallback;
    int volume = 255;
    uint32_t logVolume;
    std::atomic<bool> isRunning = false;
    trackChangedCallback trackChanged;
    std::mutex runningMutex;

    bool hasTrack(void) { return currentTrack != nullptr; }
    void setVolume(uint32_t volume);
    void handleLoad(std::shared_ptr<TrackReference> track, std::function<void()> &trackLoadedCallback, uint32_t position_ms, bool isPaused);
    void pause();
    void cancelCurrentTrack();
    void seekMs(size_t positionMs);
    void feedPCM(uint8_t *data, size_t len);
    void play();
    void stop();
};

#endif
