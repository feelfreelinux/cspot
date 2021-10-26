#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>

#include "Utils.h"
#include "MercuryManager.h"
#include "TrackReference.h"
#include "Session.h"
#include "SpotifyTrack.h"
#include "AudioSink.h"
#include <mutex>
#include "Queue.h"
#include "Task.h"

class Player : public Task {
private:
    std::shared_ptr<MercuryManager> manager;
    std::shared_ptr<SpotifyTrack> currentTrack = nullptr;
    std::shared_ptr<AudioSink> audioSink;
    std::mutex loadTrackMutex;
    // @TODO: Use some actual structure here
    Queue<std::shared_ptr<SpotifyTrack>> trackQueue;
    void runTask();

public:
    Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink);
    std::function<void()> endOfFileCallback;
    int volume = 255;
    uint32_t logVolume;
    std::atomic<bool> isRunning = false;
    trackChangedCallback trackChanged;
    std::mutex runningMutex;

    void setVolume(uint32_t volume);
    void handleLoad(std::shared_ptr<TrackReference> track, std::function<void()> &trackLoadedCallback, uint32_t position_ms, bool isPaused);
    void pause();
    void cancelCurrentTrack();
    void seekMs(size_t positionMs);
    void feedPCM(std::vector<uint8_t> &data);
    void play();
    void stop();

};

#endif
