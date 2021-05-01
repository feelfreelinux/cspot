#include "SpotifyTrack.h"
#include "unistd.h"
#include "MercuryManager.h"
#include <cassert>
#include "CspotAssert.h"
#include "Logger.h"

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

bool SpotifyTrack::countryListContains(std::string countryList, std::string country)
{
    for (int x = 0; x < countryList.size(); x += 2)
    {
        if (countryList.substr(x, 2) == country)
        {
            return true;
        }
    }
    return false;
}

bool SpotifyTrack::canPlayTrack(std::vector<Restriction> &restrictions)
{
    for (int x = 0; x < restrictions.size(); x++)
    {
        if (restrictions[x].countries_allowed.has_value())
        {
            return countryListContains(restrictions[x].countries_allowed.value(), manager->countryCode);
        }

        if (restrictions[x].countries_forbidden.has_value())
        {
            return !countryListContains(restrictions[x].countries_forbidden.value(), manager->countryCode);
        }
    }

    return true;
}

void SpotifyTrack::trackInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    if (this->fileId.size() != 0)
        return;
    CSPOT_ASSERT(response->parts.size() > 0, "response->parts.size() must be greater than 0");

    trackInfo = decodePb<Track>(response->parts[0]);

    CSPOT_LOG(info, "Track name: %s", trackInfo.name.value().c_str());
    CSPOT_LOG(debug, "trackInfo.restriction.size() = %d", trackInfo.restriction.size());
    int altIndex = 0;
    while (!canPlayTrack(trackInfo.restriction))
    {
        trackInfo.restriction = trackInfo.alternative[altIndex].restriction;
        trackInfo.gid = trackInfo.alternative[altIndex].gid;
        trackInfo.file = trackInfo.alternative[altIndex].file;
        altIndex++;
        CSPOT_LOG(info, "Trying alternative %d", altIndex);
    }
    auto trackId = trackInfo.gid.value();
    this->fileId = std::vector<uint8_t>();

    // TODO: option to set file quality
    for (int x = 0; x < trackInfo.file.size(); x++)
    {
        if (trackInfo.file[x].format == AudioFormat::OGG_VORBIS_320)
        {
            this->fileId = trackInfo.file[x].file_id.value();
        }
    }

    this->requestAudioKey(this->fileId, trackId, trackInfo.duration.value());
}

void SpotifyTrack::episodeInformationCallback(std::unique_ptr<MercuryResponse> response)
{
    if (this->fileId.size() != 0)
        return;
    CSPOT_LOG(debug, "Got to episode");
    CSPOT_ASSERT(response->parts.size() > 0, "response->parts.size() must be greater than 0");
    episodeInfo = decodePb<Episode>(response->parts[0]);

    CSPOT_LOG(info, "--- Episode name: %s", episodeInfo.name.value().c_str());

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
            CSPOT_LOG(info, "Successfully got audio key!");
            auto audioKey = std::vector<uint8_t>(res.begin() + 4, res.end());
            if (this->fileId.size() > 0)
            {
                this->audioStream = std::make_unique<ChunkedAudioStream>(this->fileId, audioKey, trackDuration, this->manager, 0);
                loadedTrackCallback();
            }
            else
            {
                CSPOT_LOG(error, "Error while fetching audiokey...");
            }
        }
        else
        {
            auto code = ntohs(extract<uint16_t>(res, 4));
            CSPOT_LOG(error, "Error while fetching audiokey, error code: %d", code);
        }
    };

    this->manager->requestAudioKey(trackId, fileId, audioKeyLambda);
}
