#include "SpotifyTrack.h"
#include "unistd.h"
#include "MercuryManager.h"
#include <cassert>
#include "esp_system.h"

SpotifyTrack::SpotifyTrack(std::shared_ptr<MercuryManager> manager, std::vector<uint8_t> &gid)
{
    this->manager = manager;
    this->fileId = std::vector<uint8_t>();
    this->responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->trackInformationCallback(std::move(res));
    };

    std::cout << "hm://metadata/3/track/" << bytesToHexString(gid) << std::endl;

    this->reqSeqNum = this->manager->execute(MercuryType::GET, "hm://metadata/3/track/" + bytesToHexString(gid), responseLambda);
}

SpotifyTrack::~SpotifyTrack()
{
    this->manager->unregisterMercuryCallback(this->reqSeqNum);
    this->manager->freeAudioKeyCallback();
}

void SpotifyTrack::trackInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    if (this->fileId.size() != 0)
        return;
    //assert("response->parts.size() must be greater than 0", (response->parts.size() > 0));
    PBWrapper<Track> trackInfo(response->parts[0]);
    std::cout << "--- Track name: " << std::string(trackInfo->name) << std::endl;
    printf("(_)--- Free memory %d\n", esp_get_free_heap_size());
    auto trackId = std::vector<uint8_t>(trackInfo->gid->bytes, trackInfo->gid->bytes + trackInfo->gid->size);

    // TODO: option to set file quality
    this->fileId = std::vector<uint8_t>(trackInfo->file[0].file_id->bytes, trackInfo->file[0].file_id->bytes + trackInfo->file[0].file_id->size);

    // We do not want the lambda to capture trackInfo since it is a PBWrapper and it does not have a copy constructor.
    // It is easier just to copy the one number.
    auto trackDuration = trackInfo->duration;
    audioKeyCallback audioKeyLambda = [=](bool success, std::vector<uint8_t> res) {
        if (success)
        {
            printf("Successfully got audio key!\n");
            auto audioKey = std::vector<uint8_t>(res.begin() + 4, res.end());
            if (this->fileId.size() > 0)
            {
                this->audioStream = std::make_unique<ChunkedAudioStream>(this->fileId, audioKey, trackDuration, this->manager, 0);
                loadedTrackCallback();
            }
            else
            {
                printf("Error while fetching audiokey...\n");
            }
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
