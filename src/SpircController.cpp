#include "SpircController.h"

const char *swVersion = "2.1.0";
const char *name = "CSpot";

SpircController::SpircController(std::shared_ptr<MercuryManager> manager, std::string username)
{
    this->manager = manager;
    this->state = {};
    this->state.has_repeat = true;
    this->state.repeat = false;
    this->state.has_shuffle = true;
    this->state.shuffle = false;
    this->state.has_status = true;
    this->state.shuffle = PlayStatus_kPlayStatusStop;
    this->state.has_position_measured_at = true;
    this->state.position_measured_at = 0;
    this->state.has_position_ms = true;
    this->state.position_ms = 0;

    this->deviceState = {};
    this->deviceState.sw_version = (char*) swVersion;
    this->deviceState.name = (char*) name;
    this->deviceState.volume = 32;
    this->deviceState.can_play = true;
    this->deviceState.is_active = false;
    this->deviceState.has_can_play = true;
    this->deviceState.has_is_active = true;

    this->username = username;

    addCapability(CapabilityType_kCanBePlayer, 1);
    addCapability(CapabilityType_kDeviceType, 4);
    addCapability(CapabilityType_kGaiaEqConnectId, 1);
    addCapability(CapabilityType_kSupportsLogout, 0);
    addCapability(CapabilityType_kIsObservable, 1);
    addCapability(CapabilityType_kVolumeSteps, 64);
    addCapability(CapabilityType_kSupportedContexts, -1, 
        std::vector<std::string>({
            "album", "playlist", "search", "inbox", 
            "toplist", "starred", "publishedstarred", "track"}));
    addCapability(CapabilityType_kSupportedTypes, -1, 
        std::vector<std::string>({
            "audio/local","audio/track", "local", "track"}));
    this->deviceState.capabilities_count = 8;

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        // this->trackInformationCallback(std::move(res));
        sendCmd(MessageType_kMessageTypeHello);
        printf("Sent hello!\n");

    };

    mercuryCallback subLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->handleFrame(res->parts[0]);
    };

    manager->execute(MercuryType::SUB, "hm://remote/user/" + username + "/", responseLambda, subLambda);
}

void SpircController::handleFrame(std::vector<uint8_t> &data) {
    printf("Got frame!\n");
    auto receivedFrame = decodePB<Frame>(Frame_fields, data);

    switch(receivedFrame.typ) {
        case MessageType_kMessageTypeNotify:
            printf("Notify frame\n");
            break;
        case MessageType_kMessageTypeLoad:
            printf("Load frame!\n");
            deviceState.is_active = true;
            deviceState.became_active_at = getCurrentTimestamp();
            state.playing_track_index = receivedFrame.state.playing_track_index;
            state.status = PlayStatus_kPlayStatusPlay;
            state.context_uri = receivedFrame.state.context_uri;
            state.track = receivedFrame.state.track;
            this->notify();

            break;
    }
}

void SpircController::notify() {
    this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircController::sendCmd(MessageType typ) {
    Frame frame = {};
    frame.version = 1;
    frame.ident = (char*) "352198fd329622137e14901634264e6f332e2422";
    frame.seq_nr = this->seqNum;
    frame.protocol_version = (char*) "2.0.0";
    frame.state = this->state;
    frame.device_state = this->deviceState;
    frame.typ = typ;
    frame.state_update_id = getCurrentTimestamp();
    frame.has_version = true;
    frame.has_seq_nr = true;
    frame.has_state = true;
    frame.has_device_state = true;
    frame.has_typ = true;
    frame.has_state_update_id = true;

    this->seqNum += 1;
    auto encodedFrame = encodePB(Frame_fields, &frame);

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        // this->trackInformationCallback(std::move(res));
    };
    auto parts = mercuryParts({encodedFrame});
    this->manager->execute(MercuryType::SEND, "hm://remote/user/" + this->username + "/", responseLambda, parts);
}

void SpircController::addCapability(CapabilityType typ, int intValue, std::vector<std::string> stringValue)
{
    this->deviceState.capabilities[capabilitiyIndex].has_typ = true;
    this->deviceState.capabilities[capabilitiyIndex].typ = typ;

    if (intValue != -1)
    {
        this->deviceState.capabilities[capabilitiyIndex].intValue[0] = intValue;
        this->deviceState.capabilities[capabilitiyIndex].intValue_count = 1;
    }
    else
    {
        this->deviceState.capabilities[capabilitiyIndex].intValue_count = 0;
    }

    for (int x; x < stringValue.size(); x++)
    {
        stringValue[x].copy(this->deviceState.capabilities[capabilitiyIndex].stringValue[x], stringValue[x].size());
        this->deviceState.capabilities[capabilitiyIndex].stringValue[x][stringValue[x].size()] = '\0';
    }

    this->deviceState.capabilities[capabilitiyIndex].stringValue_count = stringValue.size();
    this->capabilitiyIndex += 1;
}