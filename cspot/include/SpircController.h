#ifndef SPIRCCONTROLLER_H
#define SPIRCCONTROLLER_H

#include <vector>
#include <string>
#include <functional>
#include "Utils.h"
#include "MercuryManager.h"
#include "Session.h"
#include "PlayerState.h"
#include "SpotifyTrack.h"
#include "ConstantParameters.h"
#include "Player.h"
#include "ConfigJSON.h"
#include <cassert>
#include <variant>

enum class CSpotEventType {
    PLAY_PAUSE,
    VOLUME,
    TRACK_INFO,
    DISC,
    NEXT,
    PREV,
    SEEK,
	LOAD,
    PLAYBACK_START
};

struct CSpotEvent {
    CSpotEventType eventType;
    std::variant<TrackInfo, int, bool> data;
};

typedef std::function<void(CSpotEvent&)> cspotEventHandler;

class SpircController {
private:
    std::shared_ptr<MercuryManager> manager;
    std::string username;
    bool firstFrame = true;
    std::unique_ptr<Player> player;
    std::unique_ptr<PlayerState> state;
    std::shared_ptr<AudioSink> audioSink;
    std::shared_ptr<ConfigJSON> config;

    cspotEventHandler eventHandler;
    void sendCmd(MessageType typ);
    void notify();
	void sendEvent(CSpotEventType eventType, std::variant<TrackInfo, int, bool> data = 0);
    void handleFrame(std::vector<uint8_t> &data);
    void loadTrack(uint32_t position_ms = 0, bool isPaused = 0);
public:
    SpircController(std::shared_ptr<MercuryManager> manager, std::string username, std::shared_ptr<AudioSink> audioSink);
    ~SpircController();
    void subscribe();

    /** 
     * @brief Pauses / Plays current song
     *
     * Calling this function will pause or resume playback, setting the 
     * necessary state value and notifying spotify SPIRC.
     *
     * @param pause if true pause content, play otherwise
     */
    void setPause(bool pause, bool notifyPlayer = true);

    /** 
     * @brief Toggle Play/Pause
     */
	void playToggle();

    /** 
     * @brief Notifies spotify servers about volume change
     *
     * @param volume int between 0 and `MAX_VOLUME`
     */
    void setRemoteVolume(int volume);

	/** 
     * @brief Set device volume and notifies spotify
     *
     * @param volume int between 0 and `MAX_VOLUME`
     */
	void setVolume(int volume);

	/** 
     * @brief change volume by a given value
     *
     * @param volume int between 0 and `MAX_VOLUME`
     */
    void adjustVolume(int by);

    /** 
     * @brief Goes back to previous track and notfies spotify SPIRC
     */
    void prevSong();

    /** 
    * @brief Skips to next track and notfies spotify SPIRC
    */
    void nextSong();
    void setEventHandler(cspotEventHandler handler);
    void stopPlayer();

    /** 
     * @brief Disconnect players and notify
     */
    void disconnect();
};

#endif
