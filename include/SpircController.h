#ifndef SPIRCCONTROLLER_H
#define SPIRCCONTROLLER_H

#include <vector>
#include <string>
#include "Utils.h"
#include "MercuryManager.h"
#include "spirc.pb.h"
#include "PBUtils.h"
#include "Session.h"

class SpircController {
private:
    uint32_t seqNum;
    std::shared_ptr<MercuryManager> manager;
    State state;
    DeviceState deviceState;
    std::string username;
    uint8_t capabilitiyIndex = 0;

    void addCapability(CapabilityType typ, int intValue = -1, std::vector<std::string> stringsValue = std::vector<std::string>());
    void sendCmd(MessageType typ);
    void notify();

    void handleFrame(std::vector<uint8_t> &data);
public:
    SpircController(std::shared_ptr<MercuryManager> manager, std::string username);
};

#endif