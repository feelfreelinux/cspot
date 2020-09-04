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
#include "NamedPipeAudioSink.h"

class Player {
private:
    std::shared_ptr<MercuryManager> manager;
    SpotifyTrack* currentTrack = nullptr;
    std::shared_ptr<AudioSink> audioSink;

public:
    Player(std::shared_ptr<MercuryManager> manager);
    void handleLoad(TrackRef* track, std::function<void()> &trackLoadedCallback);
    void pause();
    void seekMs(size_t positionMs);
    void play();

};

#endif