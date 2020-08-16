#ifndef MERCURY_MANAGER_H
#define MERCURY_MANAGER_H

#include <map>
#include <string>
#include <functional>
#include <vector>
#include "ShannonConnection.h"
#include "Packet.h"
#include "Utils.h"
#include "PBUtils.h"
#include "mercury.pb.h"
#include <stdint.h>
#include <memory>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#endif

typedef std::vector<std::vector<uint8_t>> mercuryParts;
typedef const std::function<void(int, mercuryParts)> mercuryCallback;
#define PING 0x04
#define PONG_ACK 0x4a
#define AUDIO_CHUNK_REQUEST_COMMAND 0x08
#define AUDIO_CHUNK_SUCCESS_RESPONSE 0x09
#define AUDIO_CHUNK_FAILURE_RESPONSE 0x0A
#define AUDIO_KEY_REQUEST_COMMAND 0x0C
#define AUDIO_KEY_SUCCESS_RESPONSE 0x0D
#define AUDIO_KEY_FAILURE_RESPONSE 0x0E
#define COUNTRY_CODE_RESPONSE 0x1B

enum class RequestType {
  SUB = 0xb3,
  UNSUB = 0xb4,
  SEND = 0xb2,
  GET = 0xb2
};

class MercuryManager
{
private:
    std::map<int64_t, mercuryCallback*> callbacks;
    std::map<std::string, mercuryCallback*> subscriptions;
    std::shared_ptr<ShannonConnection> conn;
    int64_t sequenceId;
public:
    MercuryManager(std::shared_ptr<ShannonConnection> conn);
    void execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription, mercuryParts &payload);
    void execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription);
    void execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryParts &payload);
    void execute(RequestType method, std::string uri, mercuryCallback &callback);
};

#endif