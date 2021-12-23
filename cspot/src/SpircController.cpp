#include "SpircController.h"
#include "ConfigJSON.h"
#include "Logger.h"
#include "SpotifyTrack.h"

SpircController::SpircController(std::shared_ptr<MercuryManager> manager,
                                 std::string username,
                                 std::shared_ptr<AudioSink> audioSink) {

    this->manager = manager;
    this->player = std::make_unique<Player>(manager, audioSink);
    this->state = std::make_unique<PlayerState>(manager->timeProvider);
    this->username = username;

    player->endOfFileCallback = [=]() {
        if (state->nextTrack()) {
            loadTrack();
        }
    };

    player->setVolume(configMan->volume);
    subscribe();
}

void SpircController::subscribe() {
    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        // this->trackInformationCallback(std::move(res));
        sendCmd(MessageType::kMessageTypeHello);
        CSPOT_LOG(debug, "Sent kMessageTypeHello!");
    };
    mercuryCallback subLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->handleFrame(res->parts[0]);
    };

    manager->execute(MercuryType::SUB,
                     "hm://remote/user/" + this->username + "/", responseLambda,
                     subLambda);
}

void SpircController::setPause(bool isPaused, bool notifyPlayer) {
	sendEvent(CSpotEventType::PLAY_PAUSE, isPaused);
    if (isPaused) {
        CSPOT_LOG(debug, "External pause command");
        if (notifyPlayer) player->pause();
        state->setPlaybackState(PlaybackState::Paused);
    } else {
        CSPOT_LOG(debug, "External play command");
        if (notifyPlayer) player->play();
        state->setPlaybackState(PlaybackState::Playing);
    }
    notify();
}

void SpircController::disconnect(void) {
    player->cancelCurrentTrack();
	stopPlayer();
    state->setActive(false);
    notify();
	sendEvent(CSpotEventType::DISC);
}

void SpircController::playToggle() {
	if (state->innerFrame.state->status.value() == PlayStatus::kPlayStatusPause) {
		setPause(false);
	} else {
		setPause(true);
	}
}

void SpircController::adjustVolume(int by) {
	if (state->innerFrame.device_state->volume.has_value()) {
		int volume = state->innerFrame.device_state->volume.value() + by;
		if (volume < 0) volume = 0;
		else if (volume > MAX_VOLUME) volume = MAX_VOLUME;
		setVolume(volume);
	}
}

void SpircController::setVolume(int volume) {
	setRemoteVolume(volume);
    player->setVolume(volume);
    configMan->save();
}

void SpircController::setRemoteVolume(int volume) {
    state->setVolume(volume);
    notify();
}

void SpircController::nextSong() {
    if (state->nextTrack()) {
        loadTrack();
    } else {
        player->cancelCurrentTrack();
    }
    notify();
}

void SpircController::prevSong() {
    state->prevTrack();
    loadTrack();
    notify();
}

void SpircController::handleFrame(std::vector<uint8_t> &data) {
    state->remoteFrame = decodePb<Frame>(data);

    switch (state->remoteFrame.typ.value()) {
    case MessageType::kMessageTypeNotify: {
        CSPOT_LOG(debug, "Notify frame");
        // Pause the playback if another player took control
        if (state->isActive() &&
            state->remoteFrame.device_state->is_active.value()) {
			disconnect();
        }
        break;
    }
    case MessageType::kMessageTypeSeek: {
        CSPOT_LOG(debug, "Seek command");
		sendEvent(CSpotEventType::SEEK, (int) state->remoteFrame.position.value());
        state->updatePositionMs(state->remoteFrame.position.value());
        this->player->seekMs(state->remoteFrame.position.value());
        notify();
        break;
    }
    case MessageType::kMessageTypeVolume:
		sendEvent(CSpotEventType::VOLUME, (int) state->remoteFrame.volume.value());
		setVolume(state->remoteFrame.volume.value());
        break;
    case MessageType::kMessageTypePause:
        setPause(true);
        break;
    case MessageType::kMessageTypePlay:
        setPause(false);
        break;
    case MessageType::kMessageTypeNext:
		sendEvent(CSpotEventType::NEXT);
        nextSong();
        break;
    case MessageType::kMessageTypePrev:
		sendEvent(CSpotEventType::PREV);
        prevSong();
        break;
    case MessageType::kMessageTypeLoad: {
        CSPOT_LOG(debug, "Load frame!");

        state->setActive(true);

        // Every sane person on the planet would expect std::move to work here.
        // And it does... on every single platform EXCEPT for ESP32 for some
        // reason. For which it corrupts memory and makes printf fail. so yeah.
        // its cursed.
        state->updateTracks();

        // bool isPaused = (state->remoteFrame.state->status.value() ==
        // PlayStatus::kPlayStatusPlay) ? false : true;
        loadTrack(state->remoteFrame.state->position_ms.value(), false);
        state->updatePositionMs(state->remoteFrame.state->position_ms.value());

        this->notify();
        break;
    }
    case MessageType::kMessageTypeReplace: {
        CSPOT_LOG(debug, "Got replace frame!");
        break;
    }
    case MessageType::kMessageTypeShuffle: {
        CSPOT_LOG(debug, "Got shuffle frame");
        state->setShuffle(state->remoteFrame.state->shuffle.value());
        this->notify();
        break;
    }
    case MessageType::kMessageTypeRepeat: {
        CSPOT_LOG(debug, "Got repeat frame");
        state->setRepeat(state->remoteFrame.state->repeat.value());
        this->notify();
        break;
    }
    default:
        break;
    }
}

void SpircController::loadTrack(uint32_t position_ms, bool isPaused) {
    state->setPlaybackState(PlaybackState::Loading);
    std::function<void()> loadedLambda = [=]() {
        // Loading finished, notify that playback started
        setPause(isPaused, false);
    };

    player->handleLoad(state->getCurrentTrack(), loadedLambda, position_ms,
                       isPaused);
}

void SpircController::notify() {
    this->sendCmd(MessageType::kMessageTypeNotify);
}

void SpircController::sendEvent(CSpotEventType eventType, std::variant<TrackInfo, int, bool> data) {
    if (eventHandler != nullptr) {
        CSpotEvent event = {
            .eventType = eventType,
			.data = data,
        };

        eventHandler(event);
    }
}

void SpircController::setEventHandler(cspotEventHandler callback) {
    this->eventHandler = callback;

    player->trackChanged = ([this](TrackInfo &track) {
            TrackInfo info;
            info.album = track.album;
            info.artist = track.artist;
            info.imageUrl = track.imageUrl;
            info.name = track.name;

			this->sendEvent(CSpotEventType::TRACK_INFO, info);
    });
}

void SpircController::stopPlayer() { this->player->stop(); }

void SpircController::sendCmd(MessageType typ) {
    // Serialize current player state
    auto encodedFrame = state->encodeCurrentFrame(typ);

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
    };
    auto parts = mercuryParts({encodedFrame});
    this->manager->execute(MercuryType::SEND,
                           "hm://remote/user/" + this->username + "/",
                           responseLambda, parts);
}
