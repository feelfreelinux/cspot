#include "SpotifyTrack.h"
#include "unistd.h"
#include "aes.h"
#include "MercuryManager.h"

std::vector<uint8_t> audioAESIV({0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93});

SpotifyTrack::SpotifyTrack(std::shared_ptr<MercuryManager> manager)
{
    this->manager = manager;
    this->pos = 167;
    this->chunkBuffer = std::vector<std::vector<uint8_t>>();

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->trackInformationCallback(std::move(res));
    };
    pthread_mutex_init(&this->writeMutex, NULL);

    this->manager->execute(MercuryType::GET, "hm://metadata/3/track/93bc414a606747b2b612491ef83d5a3e", responseLambda);
    // oggFile = std::ofstream("asd.ogg", std::ios_base::out | std::ios_base::binary);
}

std::vector<uint8_t> SpotifyTrack::read(size_t bytes)
{

    auto toRead = bytes;
    auto res = std::vector<uint8_t>();
    while (this->audioKey.size() == 0);

    while (res.size() < bytes)
    {
        int16_t chunk = this->pos / AUDIO_CHUNK_SIZE;
        int32_t offset = this->pos % AUDIO_CHUNK_SIZE;

        printf("Read called \n");
        pthread_mutex_lock(&this->writeMutex);
        if (this->chunkBuffer.size() == 0)
        {
            printf("NON! \n");
            if (!loadingChunks)
            {
                audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> r2es) {
                    this->processAudioChunk(success, r2es);
                };
                this->manager->fetchAudioChunk(fileId, this->currentChunk + this->chunkBuffer.size(), audioChunkLambda);
            }
            // Wait for chunk
            usleep(100000);
        }
        else if (toRead < this->chunkBuffer[0].size() - offset)
        {
            printf("Read begin, from %d to %d\n", offset, offset + toRead);
            res.insert(res.end(), this->chunkBuffer[0].begin() + offset, this->chunkBuffer[0].begin() + offset + toRead);
            this->pos += toRead;
        }
        else
        {
            printf("NON2 \n");
            printf("Read begin, from %d to %d\n", offset, offset + toRead);
            printf("Size %d\n", this->chunkBuffer[0].size());
            res.insert(res.end(), this->chunkBuffer[0].begin() + offset, this->chunkBuffer[0].end());
            this->pos += this->chunkBuffer[0].size() - offset;
            printf("Read %d\n", this->chunkBuffer[0].size() - offset);
            toRead -= this->chunkBuffer[0].size() - offset;

            this->chunkBuffer.erase(this->chunkBuffer.begin());
            this->currentChunk += 1;

            if (!this->loadingChunks)
            {
                audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> r2es) {
                    this->processAudioChunk(success, r2es);
                };
                this->manager->fetchAudioChunk(fileId, this->currentChunk + this->chunkBuffer.size(), audioChunkLambda);
            }
        }
        pthread_mutex_unlock(&this->writeMutex);
    }

    return res;
}

void SpotifyTrack::processAudioChunk(bool status, std::vector<uint8_t> res)
{
    audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> r2es) {
        this->processAudioChunk(success, r2es);
    };

    if (status)
    {
        // First packet is always the header
        if (this->currentChunkHeader.size() == 0)
        {
            printf("Got header \n");
            auto headerSize = ntohs(extract<uint16_t>(res, 2));
            this->currentChunkHeader = std::vector<uint8_t>(res.begin() + 4, res.begin() + 4 + headerSize);
        }
        else
        {
            // Last packet has always 2 bytes
            if (res.size() == 2)
            {
                pthread_mutex_lock(&this->writeMutex);

                AES_CTR_xcrypt_buffer(&this->ctx, &this->currentChunkData[0], this->currentChunkData.size());
                if (this->currentChunk == 0)
                {
                    printf("Successfully got audio chunk %d\n", this->currentChunk);
                    this->chunkBuffer.insert(this->chunkBuffer.end(), std::vector<uint8_t>(this->currentChunkData.begin(), this->currentChunkData.end()));
                }
                else
                {

                    printf("Successfully got audio chunk %d\n", this->currentChunk);
                    this->chunkBuffer.insert(this->chunkBuffer.end(), std::vector<uint8_t>(this->currentChunkData.begin(), this->currentChunkData.end()));
                }
                pthread_mutex_unlock(&this->writeMutex);

                if (this->currentChunkData.size() < 0x20000)
                {
                    // this->oggFile.close();
                    printf("Finished! \n");
                    return;
                }
                // Bump chunk index
                this->currentChunkData = std::vector<uint8_t>();
                this->currentChunkHeader = std::vector<uint8_t>();
                if (this->chunkBuffer.size() < 10)
                {
                    this->manager->fetchAudioChunk(fileId, this->currentChunk + this->chunkBuffer.size(), audioChunkLambda);
                }
                else
                {
                    loadingChunks = false;
                }
            }
            else
            {
                this->currentChunkData.insert(this->currentChunkData.end(), res.begin() + 2, res.end());
            }
        }
    }
    else
    {
        this->oggFile.close();
        printf("Audio chunk failed\n");
    }
}

void SpotifyTrack::trackInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    auto trackInfo = decodePB<Track>(Track_fields, response->parts[0]);
    std::cout << trackInfo.file[6].format << "Track name: " << std::string(trackInfo.name) << std::endl;
    this->trackId = std::vector<uint8_t>(trackInfo.gid->bytes, trackInfo.gid->bytes + trackInfo.gid->size);
    this->fileId = std::vector<uint8_t>(trackInfo.file[0].file_id->bytes, trackInfo.file[0].file_id->bytes + trackInfo.file[0].file_id->size);

    audioKeyCallback audioKeyLambda = [=](bool success, std::vector<uint8_t> res) {
        audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> r2es) {
            this->processAudioChunk(success, r2es);
        };

        if (success)
        {
            printf("Successfully got audio key!\n");
            this->currentChunkData = std::vector<uint8_t>();
            this->currentChunkHeader = std::vector<uint8_t>();
            this->audioKey = std::vector<uint8_t>(res.begin() + 4, res.end());
            AES_init_ctx_iv(&this->ctx, &this->audioKey[0], &audioAESIV[0]);
            loadingChunks = true;
            this->manager->fetchAudioChunk(fileId, this->currentChunk, audioChunkLambda);
        }
        else
        {
            printf("Error while fetching audiokey...\n");
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
