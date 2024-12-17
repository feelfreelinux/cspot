#include "MercurySession.h"

#include <string.h>     // for memcpy
#include <memory>       // for shared_ptr
#include <mutex>        // for scoped_lock
#include <stdexcept>    // for runtime_error
#include <type_traits>  // for remove_extent_t, __underlying_type_impl<>:...
#include <utility>      // for pair
#ifndef _WIN32
#include <arpa/inet.h>  // for htons, ntohs, htonl, ntohl
#endif
#include "BellLogger.h"         // for AbstractLogger
#include "BellTask.h"           // for Task
#include "BellUtils.h"          // for BELL_SLEEP_MS
#include "Logger.h"             // for CSPOT_LOG
#include "NanoPBHelper.h"       // for pbPutString, pbDecode, pbEncode
#include "PlainConnection.h"    // for PlainConnection
#include "ShannonConnection.h"  // for ShannonConnection
#include "TimeProvider.h"       // for TimeProvider
#include "Utils.h"              // for extract, pack, hton64
#include "WrappedSemaphore.h"

using namespace cspot;

template <typename T>
T extractData(const std::vector<uint8_t>& data, size_t& pos) {
  static_assert(std::is_integral<T>::value,
                "extractData only supports integral types");

  // Check that we have enough bytes to extract
  if (pos + sizeof(T) > data.size()) {
    throw std::out_of_range("Not enough data to extract");
  }

  T value;
  memcpy(&value, &data[pos], sizeof(T));
  pos += sizeof(T);

  // Convert to host byte order based on the size of T
  if constexpr (sizeof(T) == 2) {
    return ntohs(value);
  } else if constexpr (sizeof(T) == 4) {
    return ntohl(value);
  } else if constexpr (sizeof(T) == 8) {
    return hton64(
        value);  // Assuming you have defined `hton64` similarly to `htonl` for 64-bit values
  } else {
    return 0;  //   static_assert(false, "Unsupported type size for extractData");
  }
}

MercurySession::MercurySession(std::shared_ptr<TimeProvider> timeProvider)
    : bell::Task("mercury_dispatcher", 8 * 1024, 3,
                 1) {  //double the size for reconnecting
  responseSemaphore = std::make_shared<bell::WrappedSemaphore>();
  this->timeProvider = timeProvider;
}

MercurySession::~MercurySession() {
  this->responseSemaphore->give();
  std::scoped_lock lock(this->isRunningMutex);
}

void MercurySession::runTask() {
  isRunning.store(true);
  std::scoped_lock lock(this->isRunningMutex);
  this->executeEstabilishedCallback = true;

  while (isRunning) {
    if (!processPackets()) {
      handleReconnection();
    }
  }
}

bool MercurySession::processPackets() {
  try {
    cspot::Packet packet = shanConn->recvPacket();
    CSPOT_LOG(info, "Received packet, command: %d", packet.command);
    if (static_cast<RequestType>(packet.command) == RequestType::PING) {
      timeProvider->syncWithPingPacket(packet.data);

      this->lastPingTimestamp = timeProvider->getSyncedTimestamp();
      this->shanConn->sendPacket(0x49, packet.data);
    } else if (packet.data.size()) {
      std::unique_lock<std::mutex> lock(queueMutex);
      this->packetQueue.push_back(packet);
      lock.unlock();  // Optional, the destructor will unlock it
      this->responseSemaphore->give();
    }
    return true;
  } catch (const std::runtime_error& e) {
    CSPOT_LOG(error, "Error while receiving packet: %s", e.what());
    failAllPending();  // Fail all pending requests
    return false;
  } catch (const std::exception& e) {
    CSPOT_LOG(error, "Unexpected exception: %s", e.what());
    failAllPending();  // Fail all pending requests
    return false;
  } catch (...) {
    CSPOT_LOG(error, "Unknown error occurred while receiving packet.");
    failAllPending();  // Fail all pending requests
    return false;
  }
}

void MercurySession::handleReconnection() {
  if (isReconnecting)
    return;

  isReconnecting = true;
  reconnect();
  isReconnecting = false;
}

void MercurySession::reconnect() {
  while (isRunning) {
    try {
      this->shanConn = nullptr;
      this->conn = nullptr;
      this->partials.clear();
      // Reset connections
      this->connectWithRandomAp();
      this->authenticate(this->authBlob);

      CSPOT_LOG(info, "Reconnection successful");

      BELL_SLEEP_MS(100);

      lastPingTimestamp = timeProvider->getSyncedTimestamp();
      isReconnecting = false;
      this->executeEstabilishedCallback = true;
      return;  // Successful connection, exit loop
    } catch (...) {
      CSPOT_LOG(error, "Cannot reconnect, will retry in 5s");
      BELL_SLEEP_MS(1000);
      if (!isRunning) {  // Stop retrying if session is not running
        return;
      }
    }
  }
}

void MercurySession::setConnectedHandler(
    ConnectionEstabilishedCallback callback) {
  this->connectionReadyCallback = callback;
}

bool MercurySession::triggerTimeout() {
  if (!isRunning)
    return true;
  auto currentTimestamp = timeProvider->getSyncedTimestamp();

  if (currentTimestamp - this->lastPingTimestamp > PING_TIMEOUT_MS) {
    CSPOT_LOG(debug, "Reconnection required, no ping received");
    return true;
  }

  return false;
}

void MercurySession::unregister(uint64_t sequenceId) {
  auto callback = this->callbacks.find(sequenceId);

  if (callback != this->callbacks.end()) {
    this->callbacks.erase(callback);
  }
}

void MercurySession::unregisterAudioKey(uint32_t sequenceId) {
  auto callback = this->audioKeyCallbacks.find(sequenceId);

  if (callback != this->audioKeyCallbacks.end()) {
    this->audioKeyCallbacks.erase(callback);
  }
}

void MercurySession::disconnect() {
  CSPOT_LOG(info, "Disconnecting mercury session");
  isRunning.store(false);
  std::scoped_lock lock(this->isRunningMutex);
  conn->close();
}

std::string MercurySession::getCountryCode() {
  return this->countryCode;
}

void MercurySession::handlePacket() {
  this->responseSemaphore->wait();
  std::unique_lock<std::mutex> lock(queueMutex);
  if (!packetQueue.size())
    return;
  Packet packet = std::move(*packetQueue.begin());
  packetQueue.pop_front();
  lock.unlock();  // Optional, the destructor will unlock it

  if (executeEstabilishedCallback && this->connectionReadyCallback != nullptr) {
    executeEstabilishedCallback = false;
    this->connectionReadyCallback();
  }

  switch (static_cast<RequestType>(packet.command)) {
    case RequestType::COUNTRY_CODE_RESPONSE: {
      this->countryCode = std::string();
      this->countryCode.resize(2);
      memcpy(this->countryCode.data(), packet.data.data(), 2);
      CSPOT_LOG(debug, "Received country code %s", this->countryCode.c_str());
      break;
    }
    case RequestType::AUDIO_KEY_FAILURE_RESPONSE:
    case RequestType::AUDIO_KEY_SUCCESS_RESPONSE: {
      // this->lastRequestTimestamp = -1;

      // First four bytes mark the sequence id
      auto seqId = ntohl(extract<uint32_t>(packet.data, 0));

      if (this->audioKeyCallbacks.count(seqId) > 0) {
        auto success = static_cast<RequestType>(packet.command) ==
                       RequestType::AUDIO_KEY_SUCCESS_RESPONSE;
        this->audioKeyCallbacks[seqId](success, packet.data);
      }
      break;
    }
    case RequestType::SEND:
    case RequestType::SUB:
    case RequestType::UNSUB: {
      CSPOT_LOG(debug, "Received mercury packet");
      auto response = this->decodeResponse(packet.data);
      if (!response.fail) {
        if (this->callbacks.count(response.sequenceId)) {
          uint64_t tempSequenceId = response.sequenceId;
          this->callbacks[response.sequenceId](response);
          this->callbacks.erase(this->callbacks.find(tempSequenceId));
        }
        pb_release(Header_fields, &response.mercuryHeader);
      }
      break;
    }
    case RequestType::SUBRES: {
      auto response = decodeResponse(packet.data);
      if (!response.fail) {
        std::string uri(response.mercuryHeader.uri);
        for (const auto& [subUri, callback] : subscriptions) {
          if (uri.find(subUri) != std::string::npos) {
            callback(response);
            break;
          }
        }
        pb_release(Header_fields, &response.mercuryHeader);
      }
      break;
    }
    default:
      break;
  }
}

void MercurySession::failAllPending() {
  Response response = {};
  response.fail = true;

  // Fail all callbacks
  for (auto& it : this->callbacks) {
    it.second(response);
  }

  // Remove references
  this->callbacks = {};
}

MercurySession::Response MercurySession::decodeResponse(
    const std::vector<uint8_t>& data) {
  size_t pos = 0;
  auto sequenceLength = extractData<uint16_t>(data, pos);
  uint64_t sequenceId;
  uint8_t flag;
  Response resp;
  resp.mercuryHeader = Header_init_default;
  if (sequenceLength == 2)
    sequenceId = extractData<uint16_t>(data, pos);
  else if (sequenceLength == 4)
    sequenceId = extractData<uint32_t>(data, pos);
  else if (sequenceLength == 8)
    sequenceId = extractData<uint64_t>(data, pos);
  else
    return resp;

  flag = (uint8_t)data[pos];
  pos++;
  uint16_t parts = extractData<uint16_t>(data, pos);
  auto partial = std::find_if(
      partials.begin(), partials.end(),
      [sequenceId](const Response& p) { return p.sequenceId == sequenceId; });
  if (partial == partials.end()) {
    if (flag == 2)
      return resp;
    CSPOT_LOG(debug,
              "Creating new Mercury Response, seq: %lli, flags: %i, parts: %i",
              sequenceId, flag, parts);
    this->partials.push_back(Response());
    partial = partials.end() - 1;
    partial->parts = {};
    partial->sequenceId = sequenceId;
  } else
    CSPOT_LOG(debug,
              "Adding to Mercury Response, seq: %lli, flags: %i, parts: %i",
              sequenceId, flag, parts);
  uint8_t index = 0;
  while (parts) {
    if (data.size() <= pos)
      break;
    auto partSize = extractData<uint16_t>(data, pos);
    if (partial->mercuryHeader.uri == NULL) {
      auto headerBytes = std::vector<uint8_t>(data.begin() + pos,
                                              data.begin() + pos + partSize);
      pbDecode(partial->mercuryHeader, Header_fields, headerBytes);
      pb_istream_t stream =
          pb_istream_from_buffer(&headerBytes[0], headerBytes.size());

      // Decode the message
      if (pb_decode(&stream, Header_fields, &partial->mercuryHeader) == false) {
        pb_release(Header_fields, &partial->mercuryHeader);
        partials.erase(partial);
        return resp;
      }
    } else {
      if (index >= partial->parts.size())
        partial->parts.push_back(std::vector<uint8_t>{});
      partial->parts[index].insert(partial->parts[index].end(),
                                   data.begin() + pos,
                                   data.begin() + pos + partSize);
      index++;
    }
    pos += partSize;
    parts--;
  }
  if (flag == static_cast<uint8_t>(ResponseFlag::FINAL) &&
      partial->mercuryHeader.uri != NULL) {
    resp = std::move(*partial);
    partials.erase(partial);
    resp.fail = false;
  }
  return resp;
}

void MercurySession::addSubscriptionListener(const std::string& uri,
                                             ResponseCallback subscription) {
  this->subscriptions.insert({uri, subscription});
}

uint64_t MercurySession::executeSubscription(RequestType method,
                                             const std::string& uri,
                                             ResponseCallback callback,
                                             ResponseCallback subscription,
                                             DataParts& payload) {
  while (isReconnecting)
    BELL_SLEEP_MS(100);
  CSPOT_LOG(debug, "Executing Mercury Request, type %s",
            RequestTypeMap[method].c_str());

  // Encode header
  pb_release(Header_fields, &tempMercuryHeader);
  tempMercuryHeader.uri = strdup(uri.c_str());
  tempMercuryHeader.method = strdup(RequestTypeMap[method].c_str());

  // Map logical request type to the appropriate wire request type (SEND for POST, GET, PUT)
  if (method == RequestType::GET || method == RequestType::POST ||
      method == RequestType::PUT) {
    method = RequestType::SEND;
  }

  if (method == RequestType::SUB) {
    this->subscriptions.insert({uri, subscription});
  }

  auto headerBytes = pbEncode(Header_fields, &tempMercuryHeader);
  pb_release(Header_fields, &tempMercuryHeader);

  if (callback != nullptr)
    this->callbacks.insert({sequenceId, callback});

  // Prepare the data packet structure:
  // [Sequence size] [SequenceId] [0x1] [Payloads number] [Header size] [Header] [Payloads (size + data)]
  auto sequenceIdBytes =
      prepareSequenceIdPayload(sequenceId, headerBytes, payload);

  // Bump sequence ID for the next request
  this->sequenceId += 1;

  try {
    while (isReconnecting)
      BELL_SLEEP_MS(100);
    this->shanConn->sendPacket(
        static_cast<std::underlying_type<RequestType>::type>(method),
        sequenceIdBytes);
  } catch (...) {
    // @TODO: handle disconnect
  }

  return this->sequenceId - 1;
}

std::vector<uint8_t> MercurySession::prepareSequenceIdPayload(
    uint64_t sequenceId, const std::vector<uint8_t>& headerBytes,
    const DataParts& payload) {
  // Pack sequenceId
  auto sequenceIdBytes = pack<uint64_t>(hton64(sequenceId));
  auto sequenceSizeBytes = pack<uint16_t>(htons(sequenceIdBytes.size()));

  // Initial parts of the packet
  sequenceIdBytes.insert(sequenceIdBytes.begin(), sequenceSizeBytes.begin(),
                         sequenceSizeBytes.end());
  sequenceIdBytes.push_back(0x01);

  auto payloadNum = pack<uint16_t>(htons(payload.size() + 1));
  sequenceIdBytes.insert(sequenceIdBytes.end(), payloadNum.begin(),
                         payloadNum.end());

  // Encode the header size and the header data
  auto headerSizePayload = pack<uint16_t>(htons(headerBytes.size()));
  sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(),
                         headerSizePayload.end());
  sequenceIdBytes.insert(sequenceIdBytes.end(), headerBytes.begin(),
                         headerBytes.end());

  // Encode all the payload parts
  for (const auto& part : payload) {
    headerSizePayload = pack<uint16_t>(htons(part.size()));
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(),
                           headerSizePayload.end());
    sequenceIdBytes.insert(sequenceIdBytes.end(), part.begin(), part.end());
  }

  return sequenceIdBytes;
}

uint32_t MercurySession::requestAudioKey(const std::vector<uint8_t>& trackId,
                                         const std::vector<uint8_t>& fileId,
                                         AudioKeyCallback audioCallback) {
  auto buffer = fileId;

  // Store callback
  this->audioKeyCallbacks.insert({this->audioKeySequence, audioCallback});

  // Structure: [FILEID] [TRACKID] [4 BYTES SEQUENCE ID] [0x00, 0x00]
  buffer.insert(buffer.end(), trackId.begin(), trackId.end());
  auto audioKeySequenceBuffer = pack<uint32_t>(htonl(this->audioKeySequence));
  buffer.insert(buffer.end(), audioKeySequenceBuffer.begin(),
                audioKeySequenceBuffer.end());
  auto suffix = std::vector<uint8_t>({0x00, 0x00});
  buffer.insert(buffer.end(), suffix.begin(), suffix.end());

  // Bump audio key sequence
  this->audioKeySequence += 1;

  // Used for broken connection detection
  // this->lastRequestTimestamp = timeProvider->getSyncedTimestamp();
  try {
    this->shanConn->sendPacket(
        static_cast<uint8_t>(RequestType::AUDIO_KEY_REQUEST_COMMAND), buffer);
  } catch (...) {
    // @TODO: Handle disconnect
  }
  return audioKeySequence - 1;
}
