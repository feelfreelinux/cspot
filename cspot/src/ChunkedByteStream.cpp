#include "ChunkedByteStream.h"

ChunkedByteStream::ChunkedByteStream(std::shared_ptr<MercuryManager> manager) {
    this->mercuryManager = manager;
    this->pos = 167; // spotify header size
}

void ChunkedByteStream::setFileInfo(std::vector<uint8_t> &fileId, std::vector<uint8_t> &audioKey) {
    this->audioKey = audioKey;
    this->fileId = fileId;
}

void ChunkedByteStream::setEnableLoadAhead(bool loadAhead) {
    this->loadAheadEnabled = loadAhead;
}

void ChunkedByteStream::fetchFileInformation() {
    std::shared_ptr<AudioChunk> beginChunk = mercuryManager->fetchAudioChunk(fileId, audioKey, 0, 0x4000);
    beginChunk->keepInMemory = true;
    while (beginChunk->isHeaderFileSizeLoadedSemaphore->twait() != 0);
    this->fileSize = beginChunk->headerFileSize;
    chunks.push_back(beginChunk);

    auto startPosition = (this->fileSize / 4) - 0x1000;

    // AES block size is 16, so the index must be divisible by it
    while ((startPosition * 4) % 16 != 0)
        startPosition++; // ik, ugly lol

    auto endChunk = mercuryManager->fetchAudioChunk(fileId, audioKey, startPosition, fileSize / 4);
    endChunk->keepInMemory = true;

    chunks.push_back(endChunk);
    requestChunk(0);
}

std::shared_ptr<AudioChunk> ChunkedByteStream::getChunkForPosition(size_t position) {
    // Find chunks that fit in requested position
    for (auto chunk: chunks) {
        if (chunk->startPosition <= position && chunk->endPosition > position) {
            return chunk;
        }
    }

    return nullptr;
}

std::shared_ptr<AudioChunk> ChunkedByteStream::requestChunk(uint16_t position) {
    auto chunk = this->mercuryManager->fetchAudioChunk(this->fileId, this->audioKey, position);

    // Store a reference internally
    chunks.push_back(chunk);
    return chunk;
}


size_t ChunkedByteStream::read(uint8_t *buf, size_t nbytes) {
    std::scoped_lock lock(this->readMutex);
    auto chunk = getChunkForPosition(pos);
    uint16_t chunkIndex = this->pos / AUDIO_CHUNK_SIZE;

    if (loadAheadEnabled) {
        for (auto it = chunks.begin(); it != chunks.end();) {
            if (((*it)->endPosition<pos || (*it)->startPosition>(pos + 2 * AUDIO_CHUNK_SIZE)) && !(*it)->keepInMemory) {
                it = chunks.erase(it);
            } else {
                it++;
            }
        }
    }

    // Request chunk if does not exist
    if (chunk == nullptr) {
        BELL_LOG(info, "cspot", "Chunk not found, requesting %d", chunkIndex);
        chunk = this->requestChunk(chunkIndex);
    }


    if (chunk != nullptr) {
        // Wait for chunk if not loaded yet
        if (!chunk->isLoaded && !chunk->isFailed) {
            BELL_LOG(info, "cspot", "Chunk not loaded, waiting for %d", chunkIndex);
            chunk->isLoadedSemaphore->wait();
        }

        if (chunk->isFailed) return 0;

        // Attempt to read from chunk
        auto read = attemptRead(buf, nbytes, chunk);
        pos += read;

        auto nextChunkPos = ((chunkIndex + 1) * AUDIO_CHUNK_SIZE) + 1;
        if (loadAheadEnabled && nextChunkPos  < fileSize) {
            auto nextChunk = getChunkForPosition(nextChunkPos);

            if (nextChunk == nullptr) {
                // Request next chunk
                this->requestChunk(chunkIndex + 1);
            }
        }

        return read;
    }
    return 0;
}

size_t ChunkedByteStream::attemptRead(uint8_t *buffer, size_t bytes, std::shared_ptr<AudioChunk> chunk) {
    //if (!chunk->isLoaded || chunk->isFailed || chunk->startPosition >= pos || chunk->endPosition < pos) return 0;

    // Calculate how many bytes we can read from chunk
    auto offset = pos - chunk->startPosition;
    auto toRead = bytes;
    if (toRead > chunk->decryptedData.size() - offset) {
        toRead = chunk->decryptedData.size() - offset;
    }

    chunk->readData(buffer, offset, toRead);
    return toRead;
}

void ChunkedByteStream::seek(size_t nbytes) {
    std::scoped_lock lock(this->readMutex);
    pos = nbytes;


    if (getChunkForPosition(this->pos) == nullptr) {
        // Seeking might look back - therefore we preload some past data
        auto startPosition = (this->pos / 4) - (AUDIO_CHUNK_SIZE / 4);

        // AES block size is 16, so the index must be divisible by it
        while ((startPosition * 4) % 16 != 0)
            startPosition++; // ik, ugly lol

        this->chunks.push_back(mercuryManager->fetchAudioChunk(fileId, audioKey, startPosition,
                                                               startPosition + (AUDIO_CHUNK_SIZE / 4)));
    }
}

size_t ChunkedByteStream::skip(size_t nbytes) {
    std::scoped_lock lock(this->readMutex);
    pos += nbytes;
    return pos;
}

size_t ChunkedByteStream::position() {
    return pos;
}

size_t ChunkedByteStream::size() {
    return fileSize;
}

void ChunkedByteStream::close() {

}