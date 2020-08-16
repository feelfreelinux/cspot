#ifndef SPOTIFYTRACK_H
#define SPOTIFYTRACK_H

#include <vector>
#include <iostream>
#include "MercuryManager.h"
#include "PBUtils.h"
#include "metadata.pb.h"
#include "Utils.h"
#include "MercuryResponse.h"

class SpotifyTrack
{
private:
    std::shared_ptr<MercuryManager> manager;
    void trackInformationCallback(std::unique_ptr<MercuryResponse> response);
public:
    SpotifyTrack(std::shared_ptr<MercuryManager> manager);
};


#endif