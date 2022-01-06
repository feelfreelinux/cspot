#ifndef CSPOT_CHUNKEDBYTESTREAM_H
#define CSPOT_CHUNKEDBYTESTREAM_H

#include "ByteStream.h"
#include "BellLogger.h"
#include <memory>
#include "MercuryManager.h"
#include "stdint.h"

class ChunkedByteStream : public bell::ByteStream {
private:
    // AES key used for data decryption
    std::vector<uint8_t> audioKey;

    // Spotify internal fileId
    std::vector<uint8_t> fileId;

    // Buffer for storing currently read chunks
    std::vector<std::shared_ptr<AudioChunk>> chunks;

    // Current position of the read pointer
    size_t pos = 0;

    size_t fileSize = -1;

    std::mutex readMutex;

    std::atomic<bool> loadAheadEnabled = false;

    std::shared_ptr<MercuryManager> mercuryManager;

    /**
     * Returns an audio chunk for given position.
     * @param position requested position
     * @return matching audio chunk, or nullptr if no chunk is available
     */
    std::shared_ptr<AudioChunk> getChunkForPosition(size_t position);

    /**
     * Requests a new audio chunk from mercury manager. Returns its structure immediately.
     * @param position index of a chunk to request
     * @return requested chunk
     */
    std::shared_ptr<AudioChunk> requestChunk(uint16_t position);

    /**
     * Tries to read data from a given audio chunk.
     * @param buffer destination buffer
     * @param bytes number of bytes to read
     * @param chunk `AudioChunk` to read from
     * @return number of bytes read
     */
    size_t attemptRead(uint8_t *buffer, size_t bytes, std::shared_ptr<AudioChunk> chunk);
public:
    ChunkedByteStream(std::shared_ptr<MercuryManager> manager);
    ~ChunkedByteStream() {};

    /**
     * Requests first chunk from the file, and then fills file information based on its header
     */
    void fetchFileInformation();

    /**
     * Enables / disables load-ahead of chunks.
     * @param loadAhead true to enable load ahead
     */
    void setEnableLoadAhead(bool loadAhead);

    /**
     * Sets information about given spotify file, necessary for chunk request
     * @param fileId id of given audio file
     * @param audioKey audio key used for decryption
     */
    void setFileInfo(std::vector<uint8_t>& fileId, std::vector<uint8_t>& audioKey);

    // ---- ByteStream methods ----
    /**
     * Reads given amount of bytes from stream. Data is OPUS encoded.
     * @param buffer buffer to read into
     * @param size amount of bytes to read
     * @return amount of bytes read
     */
    size_t read(uint8_t *buf, size_t nbytes);

    /**
     * Seeks to given position in stream
     * @param pos position to seek to
     */
    void seek(size_t pos);

    /**
     * skip given amount of bytes in stream.
     * @param nbytes amount of bytes to skip
     */
    size_t skip(size_t nbytes);

    /**
     * Returns current position in stream.
     * @return position
     */
    size_t position();

    /**
     * Returns size of the file
     * @return bytes in file
     */
    size_t size();

    /**
     * Close the reader
     */
    void close();
};

#endif //CSPOT_CHUNKEDBYTESTREAM_H
