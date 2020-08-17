#include "SpotifyTrack.h"
#include "unistd.h"
#include "aes.h"

std::vector<uint8_t> audioAESIV({0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93});

SpotifyTrack::SpotifyTrack(std::shared_ptr<MercuryManager> manager)
{
    this->manager = manager;

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->trackInformationCallback(std::move(res));
    };

    this->manager->execute(MercuryType::GET, "hm://metadata/3/track/93bc414a606747b2b612491ef83d5a3e", responseLambda);
    oggFile = std::ofstream("asd.ogg", std::ios_base::out | std::ios_base::binary);
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

                AES_CTR_xcrypt_buffer(&this->ctx, &this->currentChunkData[0], this->currentChunkData.size());
                if (this->currentChunk == 0)
                {
                    printf("Successfully got audio chunk %d\n", this->currentChunk);
                    this->oggFile.write((char *)&this->currentChunkData[167], this->currentChunkData.size() - 167);
                }
                else
                {

                    printf("Successfully got audio chunk %d\n", this->currentChunk);
                    this->oggFile.write((char *)&this->currentChunkData[0], this->currentChunkData.size());
                }

                if (this->currentChunkData.size() < 0x20000)
                {
                    this->oggFile.close();
                    printf("Finished! \n");
                    return;
                }
                // Bump chunk index
                this->currentChunk += 1;
                this->currentChunkData = std::vector<uint8_t>();
                this->currentChunkHeader = std::vector<uint8_t>();
                this->manager->fetchAudioChunk(fileId, this->currentChunk, audioChunkLambda);
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
            this->manager->fetchAudioChunk(fileId, this->currentChunk, audioChunkLambda);
        }
        else
        {
            printf("Error while fetching audiokey...\n");
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
