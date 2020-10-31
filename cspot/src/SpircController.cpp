#include "SpircController.h"

SpircController::SpircController(std::shared_ptr<MercuryManager> manager, std::string username, std::shared_ptr<AudioSink> audioSink)
{

    this->manager = manager;
    this->player = std::make_unique<Player>(manager, audioSink);
    this->state = std::make_unique<PlayerState>();
    this->username = username;

    player->endOfFileCallback = [=]() {
        if (state->nextTrack())
        {
            loadTrack();
        }
    };

    player->setVolume(MAX_VOLUME);
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

    switch (receivedFrame->typ)
    {
    case MessageType_kMessageTypeNotify:
    {
        printf("Notify frame\n");
        // Pause the playback if another player took control
        if (state->isActive() && receivedFrame->device_state.is_active)
        {
            state->setActive(false);
            notify();
            player->cancelCurrentTrack();
        }
        break;
    }
    case MessageType_kMessageTypeSeek:
    {
        printf("Seek command\n");
        state->updatePositionMs(receivedFrame->position);
        this->player->seekMs(receivedFrame->position);
        notify();
        break;
    }
    case MessageType_kMessageTypeVolume:
        state->setVolume(receivedFrame->volume);
        player->setVolume(receivedFrame->volume);
        notify();
        break;
    case MessageType_kMessageTypePause:
    {
        printf("Pause command\n");
        player->pause();
        state->setPlaybackState(PlaybackState::Paused);
        notify();

        break;
    }
    case MessageType_kMessageTypePlay:
        player->play();
        state->setPlaybackState(PlaybackState::Playing);
        notify();
        break;
    case MessageType_kMessageTypeNext:
        if (state->nextTrack())
        {
            loadTrack();
        } else {
            player->cancelCurrentTrack();
        }
        notify();
        break;
    case MessageType_kMessageTypePrev:
        state->prevTrack();
        loadTrack();
        notify();
        break;
    case MessageType_kMessageTypeLoad:
    {
        printf("Load frame!\n");

        state->setActive(true);
        state->setPlaybackState(PlaybackState::Loading);
        state->updateTracks(std::move(receivedFrame));
        loadTrack();
        this->notify();
        break;
    }
    default:
        break;
    }
}

void SpircController::loadTrack()
{
    std::function<void()> loadedLambda = [=]() {
        // Loading finished, notify that playback started
        state->setPlaybackState(PlaybackState::Playing);
        this->notify();
    };

    player->handleLoad(state->getCurrentTrack(), loadedLambda);
}

void SpircController::notify()
{
    this->sendCmd(MessageType_kMessageTypeNotify);
}

void SpircController::sendCmd(MessageType typ)
{
    // Serialize current player state
    auto encodedFrame = state->encodeCurrentFrame(typ);

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {};
    auto parts = mercuryParts({encodedFrame});
    this->manager->execute(MercuryType::SEND, "hm://remote/user/" + this->username + "/", responseLambda, parts);
}