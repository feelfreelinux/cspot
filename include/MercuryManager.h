#ifndef MERCURY_MANAGER_H
#define MERCURY_MANAGER_H

#include <map>
#include <string>
#include <functional>
#include <vector>
#include "ShannonConnection.h"
#include "MercuryResponse.h"
#include "Packet.h"
#include "Utils.h"
#include "PBUtils.h"
#include "mercury.pb.h"
#include "Task.h"
#include <stdint.h>
#include <memory>

typedef std::function<void(std::unique_ptr<MercuryResponse>)> mercuryCallback;

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

class MercuryManager : public Task
{
private:
  std::map<uint32_t, mercuryCallback> callbacks;
  std::map<std::string, mercuryCallback> subscriptions;
  std::shared_ptr<ShannonConnection> conn;
  void runTask();
  int32_t sequenceId;

public:
  MercuryManager(std::shared_ptr<ShannonConnection> conn);
  void execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription, mercuryParts &payload);
  void execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription);
  void execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryParts &payload);
  void execute(MercuryType method, std::string uri, mercuryCallback &callback);
};

#endif