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
    trackInfo = decodePb<Track>(response->parts[0]);

    std::cout << "--- Track name: " << trackInfo.name.value() << std::endl;
    auto trackId = trackInfo.gid.value();
    printf("UDO %d\n", trackId.size());
    this->fileId = std::vector<uint8_t>();

    // TODO: option to set file quality
    for (int x = 0; x < trackInfo.file.size(); x++)
    {
        printf("ruchanie %d\n", trackInfo.file[x].format);
        if (trackInfo.file[x].format == AudioFormat::OGG_VORBIS_320)
        {
            this->fileId = trackInfo.file[x].file_id.value();
            printf("so got it %d?", this->fileId.size());
        }
    }

    this->requestAudioKey(this->fileId, trackId, trackInfo.duration.value());
}

void SpotifyTrack::episodeInformationCallback(std::unique_ptr<MercuryResponse> response)
{
        if (this->fileId.size() != 0)
        return;
    printf("Got to episode\n");
    CSPOT_ASSERT(response->parts.size() > 0, "response->parts.size() must be greater than 0");
    episodeInfo = decodePb<Episode>(response->parts[0]);

    std::cout << "--- Episode name: " << episodeInfo.name.value() << std::endl;

    this->fileId = std::vector<uint8_t>();

    // TODO: option to set file quality
    for (int x = 0; x < episodeInfo.audio.size(); x++)
    {
        if (episodeInfo.audio[x].format == AudioFormat::OGG_VORBIS_96)
        {
            this->fileId = episodeInfo.audio[x].file_id.value();
        }
    }

    this->requestAudioKey(episodeInfo.gid.value(), this->fileId, episodeInfo.duration.value());
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
            auto code = ntohs(extract<uint16_t>(res, 4));
            printf("Error while fetching audiokey... %d\n", code);
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
