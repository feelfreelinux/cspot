#include "ApResolve.h"

using namespace cspot;

ApResolve::ApResolve(std::string apOverride) 
{ 
    this->apOverride = apOverride;
}

std::string ApResolve::fetchFirstApAddress()
{
    if (apOverride != "")
    {
        return apOverride;
    }

    auto request = bell::HTTPClient::get("https://apresolve.spotify.com/");

    // parse json with nlohmann
    auto json = nlohmann::json::parse(request->body());
    return json["ap_list"][0];
}
