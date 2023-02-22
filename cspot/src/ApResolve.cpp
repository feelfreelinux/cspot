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
    std::string_view responseStr = request->body();

    // parse json with nlohmann
    auto json = nlohmann::json::parse(responseStr);
    return json["ap_list"][0];
}
