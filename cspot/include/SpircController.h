#ifndef SPIRCCONTROLLER_H
#define SPIRCCONTROLLER_H

#include <vector>
#include <string>
#include <functional>
#include "Utils.h"
#include "MercuryManager.h"
#include "ProtoHelper.h"
#include "Session.h"
#include "PlayerState.h"
#include "ConstantParameters.h"
#include "Player.h"
#include <cassert>

class SpircController {
private:
    std::shared_ptr<MercuryManager> manager;

    std::string username;
    std::unique_ptr<Player> player;
    std::unique_ptr<PlayerState> state;
    std::shared_ptr<AudioSink> audioSink;

    void sendCmd(MessageType typ);
    void notify();
    void handleFrame(std::vector<uint8_t> &data);
    void loadTrack(uint32_t position_ms);
public:
    SpircController(std::shared_ptr<MercuryManager> manager, std::string username, std::shared_ptr<AudioSink> audioSink);
    void subscribe();
};

#endif
