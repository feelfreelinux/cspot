#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <functional>

#include "Utils.h"
#include "MercuryManager.h"
#include "spirc.pb.h"
#include "PBUtils.h"
#include "Session.h"
#include "SpotifyTrack.h"
#include "AudioSink.h"
#include <mutex>

class Player {
private:
    std::shared_ptr<MercuryManager> manager;
    SpotifyTrack* currentTrack = nullptr;
    std::shared_ptr<AudioSink> audioSink;
    std::mutex loadTrackMutex;

public:
    Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink);
    std::function<void()> endOfFileCallback;
    int volume = 255;

    void setVolume(uint16_t volume);
    void handleLoad(TrackRef* track, std::function<void()> &trackLoadedCallback);
    void pause();
    void seekMs(size_t positionMs);
    void feedPCM(std::vector<uint8_t> &data);
    void play();

};

#endif