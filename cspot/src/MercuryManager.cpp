#include "MercuryManager.h"
#include <iostream>
#include "Logger.h"

std::map<MercuryType, std::string> MercuryTypeMap({
    {MercuryType::GET, "GET"},
    {MercuryType::SEND, "SEND"},
    {MercuryType::SUB, "SUB"},
    {MercuryType::UNSUB, "UNSUB"},
    });

MercuryManager::MercuryManager(std::unique_ptr<Session> session): bell::Task("mercuryManager", 6 * 1024, 1, 1)
{
    tempMercuryHeader = {};
    this->timeProvider = std::make_shared<TimeProvider>();
    this->callbacks = std::map<uint64_t, mercuryCallback>();
    this->subscriptions = std::map<std::string, mercuryCallback>();
    this->session = std::move(session);
    this->sequenceId = 0x00000001;
    this->audioChunkManager = std::make_unique<AudioChunkManager>();
    this->audioChunkSequence = 0;
    this->audioKeySequence = 0;
    this->queue = std::vector<std::unique_ptr<Packet>>();
    queueSemaphore = std::make_unique<WrappedSemaphore>(200);

    this->session->shanConn->conn->timeoutHandler = [this]() {
        return this->timeoutHandler();
    };
}

MercuryManager::~MercuryManager()
{
    //pb_release(Header_fields, &tempMercuryHeader);
}

bool MercuryManager::timeoutHandler()
{
    if (!isRunning) return true;
        
    auto currentTimestamp = timeProvider->getSyncedTimestamp();

    if (this->lastRequestTimestamp != -1 && currentTimestamp - this->lastRequestTimestamp > AUDIOCHUNK_TIMEOUT_MS)
    {
        CSPOT_LOG(debug, "Reconnection required, no mercury response");
        return true;
    }

    if (currentTimestamp - this->lastPingTimestamp > PING_TIMEOUT_MS)
    {
        CSPOT_LOG(debug, "Reconnection required, no ping received");
        return true;
    }
    return false;
}

void MercuryManager::unregisterMercuryCallback(uint64_t seqId)
{
    auto element = this->callbacks.find(seqId);
    if (element != this->callbacks.end())
    {
        this->callbacks.erase(element);
    }
}

void MercuryManager::requestAudioKey(std::vector<uint8_t> trackId, std::vector<uint8_t> fileId, audioKeyCallback& audioCallback)
{
    std::lock_guard<std::mutex> guard(reconnectionMutex);
    auto buffer = fileId;
    this->keyCallback = audioCallback;
    // Structure: [FILEID] [TRACKID] [4 BYTES SEQUENCE ID] [0x00, 0x00]
    buffer.insert(buffer.end(), trackId.begin(), trackId.end());
    auto audioKeySequence = pack<uint32_t>(htonl(this->audioKeySequence));
    buffer.insert(buffer.end(), audioKeySequence.begin(), audioKeySequence.end());
    auto suffix = std::vector<uint8_t>({ 0x00, 0x00 });
    buffer.insert(buffer.end(), suffix.begin(), suffix.end());

    // Bump audio key sequence
    this->audioKeySequence += 1;

    // Used for broken connection detection
    this->lastRequestTimestamp = timeProvider->getSyncedTimestamp();
    this->session->shanConn->sendPacket(static_cast<uint8_t>(MercuryType::AUDIO_KEY_REQUEST_COMMAND), buffer);
}

void MercuryManager::freeAudioKeyCallback()
{
    this->keyCallback = nullptr;
}

std::shared_ptr<AudioChunk> MercuryManager::fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t>& audioKey, uint16_t index)
{
    return this->fetchAudioChunk(fileId, audioKey, index * AUDIO_CHUNK_SIZE / 4, (index + 1) * AUDIO_CHUNK_SIZE / 4);
}

std::shared_ptr<AudioChunk> MercuryManager::fetchAudioChunk(std::vector<uint8_t> fileId, std::vector<uint8_t>& audioKey, uint32_t startPos, uint32_t endPos)
{
    std::lock_guard<std::mutex> guard(reconnectionMutex);
    auto sampleStartBytes = pack<uint32_t>(htonl(startPos));
    auto sampleEndBytes = pack<uint32_t>(htonl(endPos));

    auto buffer = pack<uint16_t>(htons(this->audioChunkSequence));
    auto hardcodedData = std::vector<uint8_t>(
        { 0x00, 0x01, // Channel num, currently just hardcoded to 1
         0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, // bytes magic
         0x00, 0x00, 0x9C, 0x40,
         0x00, 0x02, 0x00, 0x00 });
    buffer.insert(buffer.end(), hardcodedData.begin(), hardcodedData.end());
    buffer.insert(buffer.end(), fileId.begin(), fileId.end());
    buffer.insert(buffer.end(), sampleStartBytes.begin(), sampleStartBytes.end());
    buffer.insert(buffer.end(), sampleEndBytes.begin(), sampleEndBytes.end());

    // Bump chunk sequence
    this->audioChunkSequence += 1;
    this->session->shanConn->sendPacket(static_cast<uint8_t>(MercuryType::AUDIO_CHUNK_REQUEST_COMMAND), buffer);

    // Used for broken connection detection
    this->lastRequestTimestamp = this->timeProvider->getSyncedTimestamp();
    return this->audioChunkManager->registerNewChunk(this->audioChunkSequence - 1, audioKey, startPos, endPos);
}

void MercuryManager::reconnect()
{
    std::lock_guard<std::mutex> guard(this->reconnectionMutex);
    this->lastPingTimestamp = -1;
    this->lastRequestTimestamp = -1;
RECONNECT:
    if (!isRunning) return;
    CSPOT_LOG(debug, "Trying to reconnect...");
    try
    {
        if (this->session->shanConn->conn != nullptr)
        {
            this->session->shanConn->conn->timeoutHandler = nullptr;
        }
        this->audioChunkManager->failAllChunks();
        if (this->session->authBlob != nullptr)
        {
            this->lastAuthBlob = this->session->authBlob;
        }
        this->session = std::make_unique<Session>();
        this->session->connectWithRandomAp();
        this->session->authenticate(this->lastAuthBlob);
        this->session->shanConn->conn->timeoutHandler = [this]() {
            return this->timeoutHandler();
        };
        CSPOT_LOG(debug, "Reconnected successfuly :)");
    }
    catch (...)
    {
        CSPOT_LOG(debug, "Reconnection failed, willl retry in %d secs", RECONNECTION_RETRY_MS / 1000);
        usleep(RECONNECTION_RETRY_MS * 1000);
        goto RECONNECT;
        //reconnect();
    }
}

void MercuryManager::runTask()
{
    std::scoped_lock lock(this->runningMutex);
    // Listen for mercury replies and handle them accordingly
    isRunning = true;
    while (isRunning)
    {
        std::unique_ptr<Packet> packet;
        try
        {
            packet = this->session->shanConn->recvPacket();
        }
        catch (const std::runtime_error& e)
        {
            if (!isRunning) break;
            // Reconnection required
            this->reconnect();
            this->reconnectedCallback();
            continue;
        }
        if (static_cast<MercuryType>(packet->command) == MercuryType::PING) // @TODO: Handle time synchronization through ping
        {
            this->timeProvider->syncWithPingPacket(packet->data);

            this->lastPingTimestamp = this->timeProvider->getSyncedTimestamp();
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
            this->queueSemaphore->give();
        }
    }
}

void MercuryManager::stop() {
    std::scoped_lock stop(this->stopMutex); 
    CSPOT_LOG(debug, "Stopping mercury manager");
    isRunning = false;
    audioChunkManager->close();
    std::scoped_lock lock(this->runningMutex);
    CSPOT_LOG(debug, "mercury stopped");
}

void MercuryManager::updateQueue() {
    if (queueSemaphore->twait() == 0) {
        if (this->queue.size() > 0)
        {
            std::unique_ptr<Packet> packet = std::move(this->queue[0]);
            this->queue.erase(this->queue.begin());
            if(packet == nullptr){
                return;
            }
            CSPOT_LOG(debug, "Received packet with code %d of length %d", packet->command, packet->data.size());
            switch (static_cast<MercuryType>(packet->command))
            {
            case MercuryType::COUNTRY_CODE_RESPONSE:
            {

                memcpy(countryCode, packet->data.data(), 2);
                CSPOT_LOG(debug, "Received country code: %.2s", countryCode);
                break;
            }
            case MercuryType::AUDIO_KEY_FAILURE_RESPONSE:
            case MercuryType::AUDIO_KEY_SUCCESS_RESPONSE:
            {
                this->lastRequestTimestamp = -1;

                // First four bytes mark the sequence id
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
                CSPOT_LOG(error, "Audio Chunk failure!");
                this->audioChunkManager->handleChunkData(packet->data, true);
                this->lastRequestTimestamp = -1;
                break;
            }
            case MercuryType::SEND:
            case MercuryType::SUB:
            case MercuryType::UNSUB:
            {
                auto response = std::make_unique<MercuryResponse>(packet->data);
                if (response->parts.size() > 0)
                {
                    CSPOT_LOG(debug, " MercuryType::UNSUB response->parts[0].size() = %d", response->parts[0].size());
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

                auto uri = std::string(response->mercuryHeader.uri);
                if (this->subscriptions.count(uri) > 0)
                {
                    this->subscriptions[uri](std::move(response));
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

void MercuryManager::handleQueue()
{
    while (isRunning)
    {
        this->updateQueue();
    }
    
    std::scoped_lock lock(this->stopMutex);
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback& callback, mercuryCallback& subscription, mercuryParts& payload)
{
    if (!isRunning) return -1;
    std::lock_guard<std::mutex> guard(reconnectionMutex);
    // Construct mercury header

    CSPOT_LOG(debug, "executing MercuryType %s", MercuryTypeMap[method].c_str());
    pbPutString(uri, tempMercuryHeader.uri);
    pbPutString(MercuryTypeMap[method], tempMercuryHeader.method);

    tempMercuryHeader.has_method = true;
    tempMercuryHeader.has_uri = true;

    // GET and SEND are actually the same. Therefore the override
    // The difference between them is only in header's method
    if (method == MercuryType::GET)
    {
        method = MercuryType::SEND;
    }

    auto headerBytes = pbEncode(Header_fields, &tempMercuryHeader);

    // Register a subscription when given method is called
    if (method == MercuryType::SUB)
    {
        this->subscriptions.insert({ uri, subscription });
    }

    this->callbacks.insert({ sequenceId, callback });

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

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback& callback, mercuryParts& payload)
{
    mercuryCallback subscription = nullptr;
    return this->execute(method, uri, callback, subscription, payload);
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback& callback, mercuryCallback& subscription)
{
    auto payload = mercuryParts(0);
    return this->execute(method, uri, callback, subscription, payload);
}

uint64_t MercuryManager::execute(MercuryType method, std::string uri, mercuryCallback& callback)
{
    auto payload = mercuryParts(0);
    return this->execute(method, uri, callback, payload);
}
