#include "MercurySession.h"
#include <memory>
#include "BellLogger.h"
#include "BellTask.h"
#include "CSpotContext.h"
#include "Logger.h"

using namespace cspot;

MercurySession::MercurySession(std::shared_ptr<TimeProvider> timeProvider)
    : bell::Task("mercury_dispatcher", 4 * 1024, 0, 0) {
  this->timeProvider = timeProvider;
}

MercurySession::~MercurySession() {}

void MercurySession::runTask() {
  while (true) {
    cspot::Packet packet = {};
    try {
      packet = shanConn->recvPacket();
      CSPOT_LOG(info, "Received packet, command: %d", packet.command);
    } catch (const std::runtime_error& e) {
      CSPOT_LOG(error, "Error while receiving packet: %s", e.what());
      continue;
    }
    if (static_cast<RequestType>(packet.command) == RequestType::PING) {
      timeProvider->syncWithPingPacket(packet.data);

      this->lastPingTimestamp = timeProvider->getSyncedTimestamp();
      this->shanConn->sendPacket(0x49, packet.data);
    } else {
      this->packetQueue.push(packet);
    }
  }
}

std::string MercurySession::getCountryCode() {
  return this->countryCode;
}

void MercurySession::handlePacket() {
  Packet packet = {};

  while (true) {
    this->packetQueue.wpop(packet);

    switch (static_cast<RequestType>(packet.command)) {
      case RequestType::COUNTRY_CODE_RESPONSE: {
        this->countryCode = std::string();
        this->countryCode.reserve(2);
        memcpy(this->countryCode.data(), packet.data.data(), 2);
        CSPOT_LOG(debug, "Received country code");
        break;
      }
      case RequestType::AUDIO_KEY_FAILURE_RESPONSE:
      case RequestType::AUDIO_KEY_SUCCESS_RESPONSE: {
        // this->lastRequestTimestamp = -1;

        // First four bytes mark the sequence id
        auto seqId = ntohl(extract<uint32_t>(packet.data, 0));
        if (seqId == (this->audioKeySequence - 1) &&
            audioKeyCallback != nullptr) {
          auto success = static_cast<RequestType>(packet.command) ==
                         RequestType::AUDIO_KEY_SUCCESS_RESPONSE;
          audioKeyCallback(success, packet.data);
        }
        break;
      }
      case RequestType::SEND:
      case RequestType::SUB:
      case RequestType::UNSUB: {
        CSPOT_LOG(debug, "Received mercury packet");

        auto response = this->decodeResponse(packet.data);
        if (this->callbacks.count(response.sequenceId) > 0) {
          auto seqId = response.sequenceId;
          this->callbacks[response.sequenceId](response);
          this->callbacks.erase(this->callbacks.find(seqId));
        }
        break;
      }
      case RequestType::SUBRES: {
        auto response = decodeResponse(packet.data);

        auto uri = std::string(response.mercuryHeader.uri);
        if (this->subscriptions.count(uri) > 0) {
          this->subscriptions[uri](response);
        }
        break;
      }
      default:
        break;
    }
  }
}

MercurySession::Response MercurySession::decodeResponse(
    const std::vector<uint8_t>& data) {
  Response response = {};
  response.parts = {};

  auto sequenceLength = ntohs(extract<uint16_t>(data, 0));
  response.sequenceId = hton64(extract<uint64_t>(data, 2));

  auto partsNumber = ntohs(extract<uint16_t>(data, 11));

  auto headerSize = ntohs(extract<uint16_t>(data, 13));
  auto headerBytes =
      std::vector<uint8_t>(data.begin() + 15, data.begin() + 15 + headerSize);

  auto pos = 15 + headerSize;
  while (pos < data.size()) {
    auto partSize = ntohs(extract<uint16_t>(data, pos));

    response.parts.push_back(std::vector<uint8_t>(
        data.begin() + pos + 2, data.begin() + pos + 2 + partSize));
    pos += 2 + partSize;
  }

  pbDecode(response.mercuryHeader, Header_fields, headerBytes);

  return response;
}

uint64_t MercurySession::executeSubscription(RequestType method,
                                             const std::string& uri,
                                             ResponseCallback callback,
                                             ResponseCallback subscription,
                                             DataParts& payload) {
  CSPOT_LOG(debug, "Executing Mercury Request, type %s",
            RequestTypeMap[method].c_str());

  // Encode header
  pbPutString(uri, tempMercuryHeader.uri);
  pbPutString(RequestTypeMap[method], tempMercuryHeader.method);

  tempMercuryHeader.has_method = true;
  tempMercuryHeader.has_uri = true;

  // GET and SEND are actually the same. Therefore the override
  // The difference between them is only in header's method
  if (method == RequestType::GET) {
    method = RequestType::SEND;
  }

  if (method == RequestType::SUB) {
    this->subscriptions.insert({uri, subscription});
  }

  auto headerBytes = pbEncode(Header_fields, &tempMercuryHeader);

  this->callbacks.insert({sequenceId, callback});

  // Structure: [Sequence size] [SequenceId] [0x1] [Payloads number]
  // [Header size] [Header] [Payloads (size + data)]

  // Pack sequenceId
  auto sequenceIdBytes = pack<uint64_t>(hton64(this->sequenceId));
  auto sequenceSizeBytes = pack<uint16_t>(htons(sequenceIdBytes.size()));

  sequenceIdBytes.insert(sequenceIdBytes.begin(), sequenceSizeBytes.begin(),
                         sequenceSizeBytes.end());
  sequenceIdBytes.push_back(0x01);

  auto payloadNum = pack<uint16_t>(htons(payload.size() + 1));
  sequenceIdBytes.insert(sequenceIdBytes.end(), payloadNum.begin(),
                         payloadNum.end());

  auto headerSizePayload = pack<uint16_t>(htons(headerBytes.size()));
  sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(),
                         headerSizePayload.end());
  sequenceIdBytes.insert(sequenceIdBytes.end(), headerBytes.begin(),
                         headerBytes.end());

  // Encode all the payload parts
  for (int x = 0; x < payload.size(); x++) {
    headerSizePayload = pack<uint16_t>(htons(payload[x].size()));
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(),
                           headerSizePayload.end());
    sequenceIdBytes.insert(sequenceIdBytes.end(), payload[x].begin(),
                           payload[x].end());
  }

  // Bump sequence id
  this->sequenceId += 1;

  this->shanConn->sendPacket(
      static_cast<std::underlying_type<RequestType>::type>(method),
      sequenceIdBytes);

  return this->sequenceId - 1;
}

void MercurySession::requestAudioKey(const std::vector<uint8_t>& trackId,
                                     const std::vector<uint8_t>& fileId,
                                     AudioKeyCallback audioCallback) {
  auto buffer = fileId;
  this->audioKeyCallback = audioCallback;

  // Structure: [FILEID] [TRACKID] [4 BYTES SEQUENCE ID] [0x00, 0x00]
  buffer.insert(buffer.end(), trackId.begin(), trackId.end());
  auto audioKeySequence = pack<uint32_t>(htonl(this->audioKeySequence));
  buffer.insert(buffer.end(), audioKeySequence.begin(), audioKeySequence.end());
  auto suffix = std::vector<uint8_t>({0x00, 0x00});
  buffer.insert(buffer.end(), suffix.begin(), suffix.end());

  // Bump audio key sequence
  this->audioKeySequence += 1;

  // Used for broken connection detection
  // this->lastRequestTimestamp = timeProvider->getSyncedTimestamp();
  this->shanConn->sendPacket(
      static_cast<uint8_t>(RequestType::AUDIO_KEY_REQUEST_COMMAND), buffer);
}
