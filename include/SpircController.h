#ifndef SPIRCCONTROLLER_H
#define SPIRCCONTROLLER_H

#include <vector>
#include <string>
#include <functional>
#include "Utils.h"
#include "MercuryManager.h"
#include "spirc.pb.h"
#include "PBUtils.h"
#include "Session.h"
#include "Player.h"

class SpircController {
private:
    uint32_t seqNum = 0;
    std::shared_ptr<MercuryManager> manager;
    State state;
    DeviceState deviceState;
    Frame frame;
    std::string username;
    bool sendingLoadFrame = false;
    uint8_t capabilitiyIndex = 0;
    std::unique_ptr<Player> player;

    void addCapability(CapabilityType typ, int intValue = -1, std::vector<std::string> stringsValue = std::vector<std::string>());
    void sendCmd(MessageType typ);
    void notify();
    void handleFrame(std::vector<uint8_t> &data);
    void loadTrack();
public:
    SpircController(std::shared_ptr<MercuryManager> manager, std::string username);
};

#endif