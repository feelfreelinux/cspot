#include "SpotifyTrack.h"

SpotifyTrack::SpotifyTrack(std::shared_ptr<MercuryManager> manager)  {
    this->manager = manager;

    mercuryCallback responseLambda = [=](std::unique_ptr<MercuryResponse> res) {
        this->trackInformationCallback(std::move(res));
    };

    this->manager->execute(MercuryType::GET, "hm://metadata/3/track/e91fceedf430487286c87a59b975dd8f", responseLambda);
}

void SpotifyTrack::trackInformationCallback(std::unique_ptr<MercuryResponse> response) {

    auto trackInfo = decodePB<Track>(Track_fields, response->parts[0]);
    std::cout << "Track name: " << std::string(trackInfo.name) << std::endl;
}
