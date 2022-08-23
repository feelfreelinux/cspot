#ifndef MERCURY_MANAGER_H
#define MERCURY_MANAGER_H

#include <map>
#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include "ShannonConnection.h"
#include "MercuryResponse.h"
#include "Packet.h"
#include "Utils.h"
#include "MercuryManager.h"
#include "AudioChunk.h"
#include "AudioChunkManager.h"
#include <atomic>
#include "Task.h"
#include "platform/WrappedSemaphore.h"
#include "TimeProvider.h"
#include "Session.h"
#include <NanoPBHelper.h>
#include "protobuf/mercury.pb.h"
#include <stdint.h>
#include <memory>

#define AUDIOCHUNK_TIMEOUT_MS 5 * 1000
#define RECONNECTION_RETRY_MS 5 * 1000
#define PING_TIMEOUT_MS 2 * 60 * 1000 + 5000

typedef std::function<void(std::unique_ptr<MercuryResponse>)> mercuryCallback;
typedef std::function<void(bool, std::vector<uint8_t>)> audioKeyCallback;
typedef std::function<void()> voidCallback;

#define AUDIO_CHUNK_SIZE 0x20000

enum class MercuryType : uint8_t
{
  SUB = 0xb3,
  UNSUB = 0xb4,
  SUBRES = 0xb5,
  SEND = 0xb2,
  GET = 0xFF, // Shitty workaround, it's value is actually same as SEND
  PING = 0x04,
  PONG_ACK = 0x4a,
  AUDIO_CHUNK_REQUEST_COMMAND = 0x08,
  AUDIO_CHUNK_SUCCESS_RESPONSE = 0x09,
  AUDIO_CHUNK_FAILURE_RESPONSE = 0x0A,
  AUDIO_KEY_REQUEST_COMMAND = 0x0C,
  AUDIO_KEY_SUCCESS_RESPONSE = 0x0D,
  AUDIO_KEY_FAILURE_RESPONSE = 0x0E,
  COUNTRY_CODE_RESPONSE = 0x1B,
};

extern std::map<MercuryType, std::string> MercuryTypeMap;

class MercuryManager : public bell::Task
{
private:
  Header tempMercuryHeader;
  std::map<uint64_t, mercuryCallback> callbacks;
  std::mutex reconnectionMutex;
  std::mutex runningMutex;
  std::mutex stopMutex;
  std::map<std::string, mercuryCallback> subscriptions;
  std::unique_ptr<Session> session;
  std::shared_ptr<LoginBlob> lastAuthBlob; 
  std::unique_ptr<AudioChunkManager> audioChunkManager;
  std::vector<std::unique_ptr<Packet>> queue;
  std::unique_ptr<WrappedSemaphore> queueSemaphore;
  unsigned long long lastRequestTimestamp = -1;
  unsigned long long lastPingTimestamp = -1;
  uint64_t sequenceId;
  uint32_t audioKeySequence;
  audioKeyCallback keyCallback;

  void runTask();
public:
  MercuryManager(std::unique_ptr<Session> session);
  ~MercuryManager();
  std::atomic<bool> isRunning = false;
  voidCallback reconnectedCallback;
  uint16_t audioChunkSequence;
  std::shared_ptr<TimeProvider> timeProvider;
  char countryCode[2];

  bool timeoutHandler();
  uint64_t execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription, mercuryParts &payload);
  uint64_t execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription);
  uint64_t execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryParts &payload);
  uint64_t execute(MercuryType method, std::string uri, mercuryCallback &callback);
  void updateQueue();
  void stop();
  void handleQueue();
  void requestAudioKey(std::vector<uint8_t> trackId, std::vector<uint8_t> fileId, audioKeyCallback &audioCallback);
  std::shared_ptr<AudioChunk> fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t> &audioKey, uint16_t index);
  std::shared_ptr<AudioChunk> fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos);
  void unregisterAudioCallback(uint16_t seqId);
  void unregisterMercuryCallback(uint64_t seqId);
  void freeAudioKeyCallback();
  void reconnect();
};

#endif
