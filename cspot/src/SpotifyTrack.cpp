#include "SpotifyTrack.h"
#include "unistd.h"
#include "MercuryManager.h"
#include <cassert>
#include "CspotAssert.h"

SpotifyTrack::SpotifyTrack(std::shared_ptr<MercuryManager> manager, std::shared_ptr<TrackReference> trackReference)
{
    this->manager = manager;
    this->fileId = std::vector<uint8_t>();

    mercuryCallback trackResponseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->trackInformationCallback(std::move(res));
    };

    mercuryCallback episodeResponseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->episodeInformationCallback(std::move(res));
    };

    if (trackReference->isEpisode)
    {
        this->reqSeqNum = this->manager->execute(MercuryType::GET, "hm://metadata/3/episode/" + bytesToHexString(trackReference->gid), episodeResponseLambda);
    }
    else
    {
        this->reqSeqNum = this->manager->execute(MercuryType::GET, "hm://metadata/3/track/" + bytesToHexString(trackReference->gid), trackResponseLambda);
    }
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
    CSPOT_ASSERT(response->parts.size() > 0, "response->parts.size() must be greater than 0");
    printf("[");
    for (int x = 0; x < response->parts[0].size(); x ++) {
        printf("%d, ", response->parts[0][x]);
    }
    printf("]\n");
    trackInfo.parseFromVector(response->parts[0]);
    std::cout << "--- Track name: " << trackInfo.name << std::endl;
    auto trackId = trackInfo.gid;
    std::cout << "--- tracksNumber: " << trackInfo.file.size() << std::endl;
    this->fileId = std::vector<uint8_t>();

    // TODO: option to set file quality
    for (int x = 0; x < trackInfo.file.size(); x++)
    {
        if (trackInfo.file[x].format == AudioFormat::OGG_VORBIS_320)
        {
            this->fileId = trackInfo.file[x].file_id;
        }
    }

    this->requestAudioKey(this->fileId, trackId, trackInfo.duration);
}

void SpotifyTrack::episodeInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    // if (this->fileId.size() != 0)
    //     return;
    // CSPOT_ASSERT(response->parts.size() > 0, "response->parts.size() must be greater than 0");

    // PBWrapper<Episode> episodeInfo(response->parts[0]);
    // std::cout << "--- Episode name: " << std::string(episodeInfo->name) << std::endl;
    // auto trackId = std::vector<uint8_t>(episodeInfo->gid->bytes, episodeInfo->gid->bytes + episodeInfo->gid->size);

    // this->fileId = std::vector<uint8_t>();

    // for (int x = 0; x < episodeInfo->audio_count; x++)
    // {
    //     if (episodeInfo->audio[x].format == AudioFile_Format_OGG_VORBIS_96)
    //     {
    //         this->fileId = std::vector<uint8_t>(episodeInfo->audio[x].file_id->bytes, episodeInfo->audio[x].file_id->bytes + episodeInfo->audio[x].file_id->size);
    //     }
    // }

    // this->requestAudioKey(trackId, this->fileId, episodeInfo->duration);
}

void SpotifyTrack::requestAudioKey(std::vector<uint8_t> fileId, std::vector<uint8_t> trackId, int32_t trackDuration)
{
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
        else
        {
            printf("Error while fetching audiokey...\n");
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
