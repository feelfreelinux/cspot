#include "ApResolve.h"

using namespace cspot;

ApResolve::ApResolve(std::string apOverride) 
{ 
    this->apOverride = apOverride;
}

std::string_view ApResolve::getApList()
{
    auto request = bell::HTTPClient::get("https://apresolve.spotify.com/");
    return request->body();
}

std::string ApResolve::fetchFirstApAddress()
{
    if (apOverride != "")
    {
        return apOverride;
    }

    // parse json with nlohmann
    auto jsonData = getApList();
    auto json = nlohmann::json::parse(jsonData);
    return json["ap_list"][0];
}
