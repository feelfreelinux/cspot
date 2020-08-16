#include "MercuryManager.h"

MercuryManager::MercuryManager(std::shared_ptr<ShannonConnection> conn) {
    this->callbacks = std::map<int64_t, mercuryCallback*>();
    this->subscriptions = std::map<std::string, mercuryCallback*>();
    this->conn = conn;
    this->sequenceId = 0x0000000000000000;
}

void MercuryManager::runTask() {
    while(true) {
        auto packet = this->conn->recvPacket();
        printf("Received packet with code %d of length %d\n", packet->command, packet->data.size());
        switch(packet->command) {
            case PING:
            {
                printf("Got ping\n");
                this->conn->sendPacket(0x49, packet->data);
            }
        }
    }
}

void MercuryManager::execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription, mercuryParts &payload) {
    std::map<RequestType, std::string> requestTypeMap;
    requestTypeMap[RequestType::SEND] = "SEND";
    requestTypeMap[RequestType::GET] = "GET";
    requestTypeMap[RequestType::SUB] = "SUB";
    requestTypeMap[RequestType::UNSUB] = "UNSUB";

    // Construct mercury header
    Header mercuryHeader = {};
    mercuryHeader.uri = (char *) (uri.c_str());
    mercuryHeader.method = (char *) (requestTypeMap[method].c_str());

    auto headerBytes = encodePB(Header_fields, &mercuryHeader);

    // Register a subscription when given method is called
    if (method == RequestType::SUB) {
        this->subscriptions[uri] = &subscription;
    }

    this->callbacks[this->sequenceId] = &callback;
    
    // Structure: [Command] [SequenceId] [0x1] [Payloads number]
    // [Header size] [Header] [Payloads (size + data)]

    // Pack sequenceId
    auto sequenceIdBytes = pack<uint64_t>(htobe64(this->sequenceId));
    auto command = std::vector<uint8_t>({0x00, 0x04});
    
    sequenceIdBytes.insert(sequenceIdBytes.begin(), command.begin(), command.end());
    sequenceIdBytes.push_back(0x01);

    auto payloadNum = pack<uint32_t>(htonl(payload.size() + 1));
    sequenceIdBytes.insert(sequenceIdBytes.end(), payloadNum.begin(), payloadNum.end());
    
    auto headerSizePayload = pack<uint32_t>(htonl(headerBytes.size()));
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(), headerSizePayload.end());
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerBytes.begin(), headerBytes.end());

    // Encode all the payload parts
    for (int x = 0; x < payload.size(); x++) {
        headerSizePayload = pack<uint32_t>(htonl(payload[x].size()));
        sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(), headerSizePayload.end());
        sequenceIdBytes.insert(sequenceIdBytes.end(), payload[x].begin(), payload[x].end());
    }

    // Bump sequence id
    this->sequenceId += 1;
    this->conn->sendPacket(static_cast<std::underlying_type<RequestType>::type>(method), sequenceIdBytes);
}

void MercuryManager::execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryParts &payload) {
    mercuryCallback& subscription = nullptr;
    this->execute(method, uri, callback, subscription, payload);
}

void MercuryManager::execute(RequestType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription) {
    auto payload = mercuryParts(0);
    this->execute(method, uri, callback, subscription, payload);
}

void MercuryManager::execute(RequestType method, std::string uri, mercuryCallback &callback) {
    auto payload = mercuryParts(0);
    this->execute(method, uri, callback, payload);
}