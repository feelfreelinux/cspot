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

    this->manager->execute(MercuryType::GET, "hm://metadata/3/track/e91fceedf430487286c87a59b975dd8f", responseLambda);
}

void SpotifyTrack::processAudioChunk(bool status, std::vector<uint8_t> res)
{
    audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> res) {
        this->processAudioChunk(success, res);
    };

    if (status)
    {
        // First packet is always the header
        if (this->currentChunkHeader.size() == 0)
        {
            auto headerSize = ntohs(extract<uint16_t>(res, 2));
            this->currentChunkHeader = std::vector<uint8_t>(res.begin() + 4, res.begin() + 4 + headerSize);
        }
        else
        {
            // Last packet has always 2 bytes
            if (res.size() == 2)
            {
                if (this->currentChunkData.size() < 0x20000)
                {
                    printf("Finished! \n");
                    return;
                }
                struct AES_ctx ctx;
                AES_init_ctx_iv(&ctx, &this->audioKey[0], &audioAESIV[0]);
                AES_CTR_xcrypt_buffer(&ctx, &this->currentChunkData[0], this->currentChunkData.size());

                printf("Successfully got audio chunk %d\n", this->currentChunk);

                // Bump chunk index
                this->currentChunk += 1;
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
        audioChunkCallback audioChunkLambda = [=](bool success, std::vector<uint8_t> res) {
            this->processAudioChunk(success, res);
        };

        if (success)
        {
            printf("Successfully got audio key!\n");
            this->currentChunkData = std::vector<uint8_t>();
            this->currentChunkHeader = std::vector<uint8_t>();
            this->audioKey = std::vector<uint8_t>(res.begin() + 4, res.end());
            this->manager->fetchAudioChunk(fileId, this->currentChunk, audioChunkLambda);
        }
        else
        {
            printf("Error while fetching audiokey...\n");
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
