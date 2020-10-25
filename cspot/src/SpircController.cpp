#include "SpircController.h"
#include <cassert>
#include "Assert.h"

SpircController::SpircController(std::shared_ptr<MercuryManager> manager, std::string username, std::shared_ptr<AudioSink> audioSink)
{

    this->manager = manager;
    this->player = std::make_unique<Player>(manager, audioSink);
    this->frame = {};
    this->frame.state.has_repeat = true;
    this->frame.state.repeat = false;
    this->frame.state.has_shuffle = true;
    this->frame.state.shuffle = false;
    this->frame.state.has_status = true;
    this->frame.state.status = PlayStatus_kPlayStatusStop;
    this->frame.state.has_position_measured_at = true;
    this->frame.state.position_measured_at = 0;
    this->frame.state.has_position_ms = true;
    this->frame.state.position_ms = 0;

    this->frame.device_state.sw_version = (char *)swVersion;
    this->frame.device_state.name = (char *)defaultDeviceName;
    this->frame.device_state.volume = MAX_VOLUME;

    this->frame.device_state.can_play = true;
    this->frame.device_state.is_active = false;
    this->frame.device_state.has_volume = true;
    this->frame.device_state.has_can_play = true;
    this->frame.device_state.has_is_active = true;

    this->username = username;

    addCapability(CapabilityType_kCanBePlayer, 1);
    addCapability(CapabilityType_kDeviceType, 4);
    addCapability(CapabilityType_kGaiaEqConnectId, 1);
    addCapability(CapabilityType_kSupportsLogout, 0);
    addCapability(CapabilityType_kIsObservable, 1);
    addCapability(CapabilityType_kVolumeSteps, 64);
    addCapability(CapabilityType_kSupportedContexts, -1,
                  std::vector<std::string>({"album", "playlist", "search", "inbox",
                                            "toplist", "starred", "publishedstarred", "track"}));
    addCapability(CapabilityType_kSupportedTypes, -1,
                  std::vector<std::string>({"audio/local", "audio/track", "local", "track"}));
    this->frame.device_state.capabilities_count = 8;

    player->endOfFileCallback = [=]() {
        this->frame.state.playing_track_index++;
        loadTrack();
    };
    subscribe();
}

void SpircController::subscribe()
{
    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        // this->trackInformationCallback(std::move(res));
        sendCmd(MessageType_kMessageTypeHello);
        printf("Sent hello!\n");
    };
    mercuryCallback subLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->handleFrame(res->parts[0]);
    };

    manager->execute(MercuryType::SUB, "hm://remote/user/" + this->username + "/", responseLambda, subLambda);
}

void SpircController::handleFrame(std::vector<uint8_t> &data)
{
    printf("Got spirc frame!\n");
    PBWrapper<Frame> receivedFrame(data);

    // std::cout << std::string(receivedFrame->ident) << std::endl;

    switch (receivedFrame->typ)
    {
    case MessageType_kMessageTypeNotify:
    {
        printf("Notify frame\n");
        // Pause the playback if another player took control
        if (this->frame.device_state.is_active && receivedFrame->device_state.is_active && !sendingLoadFrame)
        {
            this->frame.device_state.is_active = false;
            this->frame.state.status = PlayStatus_kPlayStatusStop;
            notify();
        }
        break;
    }
    case MessageType_kMessageTypeSeek:
    {
        printf("Seek command\n");
        this->frame.state.position_ms = receivedFrame->position;
        this->player->seekMs(receivedFrame->position);
        this->frame.state.position_measured_at = getCurrentTimestamp();
        notify();
        break;
    }
    case MessageType_kMessageTypeVolume:
        this->frame.device_state.volume = receivedFrame->volume;
        player->setVolume((receivedFrame->volume / (double)MAX_VOLUME) * 255);
        notify();
        break;
    case MessageType_kMessageTypePause:
    {
        printf("Pause command\n");
        player->pause();
        this->frame.state.status = PlayStatus_kPlayStatusPause;
        uint32_t diff = getCurrentTimestamp() - this->frame.state.position_measured_at;
        this->frame.state.position_ms = this->frame.state.position_ms + diff;
        this->frame.state.position_measured_at = getCurrentTimestamp();
        notify();

        break;
    }
    case MessageType_kMessageTypePlay:
        player->play();
        this->frame.state.status = PlayStatus_kPlayStatusPlay;
        this->frame.state.position_measured_at = getCurrentTimestamp();
        notify();
        break;
    case MessageType_kMessageTypeNext:
        this->frame.state.playing_track_index++;
        loadTrack();
        notify();
        break;
    case MessageType_kMessageTypePrev:
        this->frame.state.playing_track_index--;
        loadTrack();
        notify();
        break;
    case MessageType_kMessageTypeLoad:
    {
        printf("Load frame!\n");
        this->sendingLoadFrame = true;
        this->frame.device_state.is_active = true;
        this->frame.device_state.became_active_at = getCurrentTimestamp();
        this->frame.device_state.has_became_active_at = true;
        this->frame.state.playing_track_index = receivedFrame->state.playing_track_index;
        this->frame.state.status = PlayStatus_kPlayStatusLoading;
        // this->frame.state.context_uri = receivedFrame->state.context_uri;

        this->frame.state.context_uri = receivedFrame->state.context_uri == nullptr ? nullptr : strdup(receivedFrame->state.context_uri);
        printf("---- Track count %d\n", receivedFrame->state.track_count);
        CSPOT_ASSERT(receivedFrame->state.track_count < 100, "receivedFrame->state.track_count cannot overflow track[100]");
        this->frame.state.track_count = receivedFrame->state.track_count;
        for (int i = 0; i < receivedFrame->state.track_count; i++)
        {
            this->frame.state.track[i].has_uri = receivedFrame->state.track[i].has_uri;
            this->frame.state.track[i].has_queued = receivedFrame->state.track[i].has_queued;
            this->frame.state.track[i].queued = receivedFrame->state.track[i].queued;
            this->frame.state.track[i].context = receivedFrame->state.track[i].context;

            memcpy(this->frame.state.track[i].uri, receivedFrame->state.track[i].uri, sizeof(receivedFrame->state.track[i].uri));
            auto result = static_cast<pb_bytes_array_t *>(
                malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(receivedFrame->state.track[i].gid->size)));
            result->size = receivedFrame->state.track[i].gid->size;
            memcpy(result->bytes, receivedFrame->state.track[i].gid->bytes, receivedFrame->state.track[i].gid->size);
            this->frame.state.track[i].gid = result;
        }
        this->frame.state.has_playing_track_index = true;

        loadTrack();
        break;
    }
    default:
        break;
    }
}

void SpircController::loadTrack()
{
    this->frame.state.position_ms = 0;
    this->frame.state.position_measured_at = getCurrentTimestamp();

    std::function<void()> loadedLambda = [=]() {
        this->frame.state.position_ms = 0;
        this->frame.state.position_measured_at = getCurrentTimestamp();
        this->frame.state.status = PlayStatus_kPlayStatusPlay;
        this->notify();
    };
    this->notify();
    // TODO: implement something sane
    CSPOT_ASSERT(this->frame.state.playing_track_index < this->frame.state.track_count, "reached end of playlist, aborting");
    player->handleLoad(&this->frame.state.track[this->frame.state.playing_track_index], loadedLambda);
}

void SpircController::notify()
{
    this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircController::sendCmd(MessageType typ)
{
    this->frame.version = 1;
    this->frame.ident = (char *)deviceId;
    this->frame.seq_nr = this->seqNum;
    this->frame.protocol_version = (char *)protocolVersion;
    this->frame.typ = typ;
    this->frame.state_update_id = getCurrentTimestamp();
    this->frame.has_version = true;
    this->frame.has_seq_nr = true;
    this->frame.recipient_count = 0;
    this->frame.has_state = true;
    this->frame.has_device_state = true;
    this->frame.has_typ = true;
    this->frame.has_state_update_id = true;

    this->seqNum += 1;
    auto encodedFrame = encodePB(Frame_fields, &this->frame);

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        // this->trackInformationCallback(std::move(res));
        this->sendingLoadFrame = false;
    };
    auto parts = mercuryParts({encodedFrame});
    this->manager->execute(MercuryType::SEND, "hm://remote/user/" + this->username + "/", responseLambda, parts);
}

void SpircController::addCapability(CapabilityType typ, int intValue, std::vector<std::string> stringValue)
{
    this->frame.device_state.capabilities[capabilitiyIndex].has_typ = true;
    this->frame.device_state.capabilities[capabilitiyIndex].typ = typ;

    if (intValue != -1)
    {
        this->frame.device_state.capabilities[capabilitiyIndex].intValue[0] = intValue;
        this->frame.device_state.capabilities[capabilitiyIndex].intValue_count = 1;
    }
    else
    {
        this->frame.device_state.capabilities[capabilitiyIndex].intValue_count = 0;
    }

    for (int x = 0; x < stringValue.size(); x++)
    {
        stringValue[x].copy(this->frame.device_state.capabilities[capabilitiyIndex].stringValue[x], stringValue[x].size());
        this->frame.device_state.capabilities[capabilitiyIndex].stringValue[x][stringValue[x].size()] = '\0';
    }

    this->frame.device_state.capabilities[capabilitiyIndex].stringValue_count = stringValue.size();
    this->capabilitiyIndex += 1;
}
