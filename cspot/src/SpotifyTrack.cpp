#include "SpotifyTrack.h"
#include "unistd.h"
#include "MercuryManager.h"
#include <cassert>

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

SpotifyTrack::~SpotifyTrack() {
    this->manager->unregisterMercuryCallback(this->reqSeqNum);
    this->manager->freeAudioKeyCallback();
}

void SpotifyTrack::trackInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    if (this->fileId.size() != 0) return;
    assert(("response->parts.size() must be greater than 0", response->parts.size() > 0));
    auto trackInfo = decodePB<Track>(Track_fields, response->parts[0]);
    std::cout  << "--- Track name: " << std::string(trackInfo.name) << std::endl;
    auto trackId = std::vector<uint8_t>(trackInfo.gid->bytes, trackInfo.gid->bytes + trackInfo.gid->size);

    // TODO: option to set file quality
    this->fileId = std::vector<uint8_t>(trackInfo.file[0].file_id->bytes, trackInfo.file[0].file_id->bytes + trackInfo.file[0].file_id->size);


    audioKeyCallback audioKeyLambda = [=](bool success, std::vector<uint8_t> res) {
        if (success)
        {
            printf("Successfully got audio key!\n");
            auto audioKey = std::vector<uint8_t>(res.begin() + 4, res.end());
            if (this->fileId.size() > 0) {
                // TODO: variable position from frame
                this->audioStream = std::make_unique<ChunkedAudioStream>(this->fileId, audioKey, trackInfo.duration, this->manager, 0);

                loadedTrackCallback(); 
            }
        }
        else
        {
            printf("Error while fetching audiokey...\n");
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
