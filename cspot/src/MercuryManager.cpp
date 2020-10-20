#include "MercuryManager.h"
#include <iostream>

std::map<MercuryType, std::string> MercuryTypeMap({
    {MercuryType::GET, "GET"},
    {MercuryType::SEND, "SEND"},
    {MercuryType::SUB, "SUB"},
    {MercuryType::UNSUB, "UNSUB"},
});

MercuryManager::MercuryManager(std::shared_ptr<Session> session)
{
    this->callbacks = std::map<uint64_t, mercuryCallback>();
    this->subscriptions = std::map<std::string, mercuryCallback>();
    this->session = session;
    this->sequenceId = 0x00000001;
    this->audioChunkManager = std::make_unique<AudioChunkManager>();
    this->audioChunkSequence = 0;
    this->audioKeySequence = 0;
    this->queue = std::vector<std::unique_ptr<Packet>>();
    queueSemaphore = std::make_unique<WrappedSemaphore>(200);

    this->session->shanConn->conn->timeoutHandler = [this]() {
        auto currentTimestamp = getCurrentTimestamp();

        if (this->lastRequestTimestamp != -1 && currentTimestamp - this->lastRequestTimestamp > AUDIOCHUNK_TIMEOUT_MS)
        {
            printf("Reconnection required, no mercury response\n");
            return true;
        }

        if (currentTimestamp - this->lastPingTimestamp > PING_TIMEOUT_MS)
        {
            printf("Reconnection required, no ping received\n");
            return true;
        }
        return false;
    };
}

void MercuryManager::unregisterMercuryCallback(uint64_t seqId)
{
    auto element = this->callbacks.find(seqId);
    if (element != this->callbacks.end())
    {
        this->callbacks.erase(element);
    }
}

void MercuryManager::requestAudioKey(std::vector<uint8_t> trackId, std::vector<uint8_t> fileId, audioKeyCallback &audioCallback)
{
    auto buffer = fileId;
    this->keyCallback = audioCallback;
    // Structure: [FILEID] [TRACKID] [4 BYTES SEQUENCE ID] [0x00, 0x00]
    buffer.insert(buffer.end(), trackId.begin(), trackId.end());
    auto audioKeySequence = pack<uint32_t>(htonl(this->audioKeySequence));
    buffer.insert(buffer.end(), audioKeySequence.begin(), audioKeySequence.end());
    auto suffix = std::vector<uint8_t>({0x00, 0x00});
    buffer.insert(buffer.end(), suffix.begin(), suffix.end());

    // Bump audio key sequence
    this->audioKeySequence += 1;

    // Used for broken connection detection
    this->lastRequestTimestamp = getCurrentTimestamp();
    this->session->shanConn->sendPacket(static_cast<uint8_t>(MercuryType::AUDIO_KEY_REQUEST_COMMAND), buffer);
}

void MercuryManager::freeAudioKeyCallback()
{
    this->keyCallback = nullptr;
}

std::shared_ptr<AudioChunk> MercuryManager::fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t> &audioKey, uint16_t index)
{
    return this->fetchAudioChunk(fileId, audioKey, index * AUDIO_CHUNK_SIZE / 4, (index + 1) * AUDIO_CHUNK_SIZE / 4);
}

std::shared_ptr<AudioChunk> MercuryManager::fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t> &audioKey, uint32_t startPos, uint32_t endPos)
{
    auto sampleStartBytes = pack<uint32_t>(htonl(startPos));
    auto sampleEndBytes = pack<uint32_t>(htonl(endPos));

    auto buffer = pack<uint16_t>(htons(this->audioChunkSequence));
    auto hardcodedData = std::vector<uint8_t>(
        {0x00, 0x01, // Channel num, currently just hardcoded to 1
         0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, // bytes magic
         0x00, 0x00, 0x9C, 0x40,
         0x00, 0x02, 0x00, 0x00});
    buffer.insert(buffer.end(), hardcodedData.begin(), hardcodedData.end());
    buffer.insert(buffer.end(), fileId.begin(), fileId.end());
    buffer.insert(buffer.end(), sampleStartBytes.begin(), sampleStartBytes.end());
    buffer.insert(buffer.end(), sampleEndBytes.begin(), sampleEndBytes.end());

    // Bump chunk sequence
    this->audioChunkSequence += 1;
    this->session->shanConn->sendPacket(static_cast<uint8_t>(MercuryType::AUDIO_CHUNK_REQUEST_COMMAND), buffer);

    // Used for broken connection detection
    this->lastRequestTimestamp = getCurrentTimestamp();
    return audioChunkManager->registerNewChunk(this->audioChunkSequence - 1, audioKey, startPos, endPos);
}

void MercuryManager::runTask()
{
    // Listen for mercury replies and handle them accordingly
    while (true)
    {
        auto packet = this->session->shanConn->recvPacket();
        if (static_cast<MercuryType>(packet->command) == MercuryType::PING)
        {
            printf("Got ping\n");
            this->lastPingTimestamp = getCurrentTimestamp();
            this->session->shanConn->sendPacket(0x49, packet->data);
        }
        else if (static_cast<MercuryType>(packet->command) == MercuryType::AUDIO_CHUNK_SUCCESS_RESPONSE)
        {
            this->lastRequestTimestamp = -1;
            this->audioChunkManager->handleChunkData(packet->data, false);
        }
        else
        {
            this->queue.push_back(std::move(packet));
            queueSemaphore->give();
        }
    }
}

void MercuryManager::handleQueue()
{
    while (true)
    {
        queueSemaphore->wait();
        if (this->queue.size() > 0)
        {
            auto packet = std::move(this->queue[0]);
            this->queue.erase(this->queue.begin());
            printf("Received packet with code %d of length %d\n", packet->command, packet->data.size());
            switch (static_cast<MercuryType>(packet->command))
            {

            case MercuryType::AUDIO_KEY_FAILURE_RESPONSE:
            case MercuryType::AUDIO_KEY_SUCCESS_RESPONSE:
            {
                this->lastRequestTimestamp = -1;
                auto seqId = ntohl(extract<uint32_t>(packet->data, 0));
                if (seqId == (this->audioKeySequence - 1) && this->keyCallback != nullptr)
                {
                    auto success = static_cast<MercuryType>(packet->command) == MercuryType::AUDIO_KEY_SUCCESS_RESPONSE;
                    this->keyCallback(success, packet->data);
                }
                break;
            }
            case MercuryType::AUDIO_CHUNK_FAILURE_RESPONSE:
            {
                printf("Audio Chunk failure!\n");
                this->audioChunkManager->handleChunkData(packet->data, true);
                this->lastRequestTimestamp = -1;
                break;
            }
            case MercuryType::SEND:
            case MercuryType::SUB:
            case MercuryType::UNSUB:
            {
                auto response = std::make_unique<MercuryResponse>(packet->data);
                std::cout << "Before calling cb 0 response->parts.size() = " << response->parts.size() << "\n";
                if (response->parts.size() > 0)
                {
                    std::cout << " MercuryType::UNSUB response->parts[0].size() " << response->parts[0].size() << "\n";
                }
                if (this->callbacks.count(response->sequenceId) > 0)
                {
                    auto seqId = response->sequenceId;
                    this->callbacks[response->sequenceId](std::move(response));
                    this->callbacks.erase(this->callbacks.find(seqId));
                }
                break;
            }
            case MercuryType::SUBRES:
            {
                auto response = std::make_unique<MercuryResponse>(packet->data);

                if (this->subscriptions.count(std::string(response->mercuryHeader->uri)) > 0)
                {
                    this->subscriptions[std::string(response->mercuryHeader->uri)](std::move(response));
                    //this->subscriptions.erase(std::string(response->mercuryHeader.uri));
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription, mercuryParts &payload)
{

    // Construct mercury header
    std::cout << MercuryTypeMap[method] << std::endl;
    Header mercuryHeader = {};
    mercuryHeader.uri = (char *)(uri.c_str());
    mercuryHeader.method = (char *)(MercuryTypeMap[method].c_str());

    // GET and SEND are actually the same. Therefore the override
    // The difference between them is only in header's method
    if (method == MercuryType::GET)
    {
        method = MercuryType::SEND;
    }

    auto headerBytes = encodePB(Header_fields, &mercuryHeader);

    // Register a subscription when given method is called
    if (method == MercuryType::SUB)
    {
        this->subscriptions.insert({uri, subscription});
    }

    this->callbacks.insert({sequenceId, callback});

    // Structure: [Sequence size] [SequenceId] [0x1] [Payloads number]
    // [Header size] [Header] [Payloads (size + data)]

    // Pack sequenceId
    auto sequenceIdBytes = pack<uint64_t>(hton64(this->sequenceId));
    auto sequenceSizeBytes = pack<uint16_t>(htons(sequenceIdBytes.size()));

    sequenceIdBytes.insert(sequenceIdBytes.begin(), sequenceSizeBytes.begin(), sequenceSizeBytes.end());
    sequenceIdBytes.push_back(0x01);

    auto payloadNum = pack<uint16_t>(htons(payload.size() + 1));
    sequenceIdBytes.insert(sequenceIdBytes.end(), payloadNum.begin(), payloadNum.end());

    auto headerSizePayload = pack<uint16_t>(htons(headerBytes.size()));
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(), headerSizePayload.end());
    sequenceIdBytes.insert(sequenceIdBytes.end(), headerBytes.begin(), headerBytes.end());

    // Encode all the payload parts
    for (int x = 0; x < payload.size(); x++)
    {
        headerSizePayload = pack<uint16_t>(htons(payload[x].size()));
        sequenceIdBytes.insert(sequenceIdBytes.end(), headerSizePayload.begin(), headerSizePayload.end());
        sequenceIdBytes.insert(sequenceIdBytes.end(), payload[x].begin(), payload[x].end());
    }

    // Bump sequence id
    this->sequenceId += 1;

    this->session->shanConn->sendPacket(static_cast<std::underlying_type<MercuryType>::type>(method), sequenceIdBytes);

    return this->sequenceId - 1;
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryParts &payload)
{
    mercuryCallback subscription = nullptr;
    return this->execute(method, uri, callback, subscription, payload);
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback &callback, mercuryCallback &subscription)
{
    auto payload = mercuryParts(0);
    return this->execute(method, uri, callback, subscription, payload);
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback &callback)
{
    auto payload = mercuryParts(0);
    return this->execute(method, uri, callback, payload);
}
