#include "ApResolve.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include "Logger.h"
#include <cJSON.h>
#include <ConfigJSON.h>
#include <random>

ApResolve::ApResolve() {}

std::string ApResolve::getApList()
{
    // hostname lookup
    struct hostent *host = gethostbyname("apresolve.spotify.com");
    struct sockaddr_in client;

    if ((host == NULL) || (host->h_addr == NULL))
    {
        CSPOT_LOG(error, "apresolve: DNS lookup error");
        throw std::runtime_error("Resolve failed");
    }
    
    // Prepare socket
    bzero(&client, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(80);
    memcpy(&client.sin_addr, host->h_addr, host->h_length);

    int sockFd = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to spotify's server
    if (connect(sockFd, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        close(sockFd);
        CSPOT_LOG(error, "Could not connect to apresolve");
        throw std::runtime_error("Resolve failed");
    }

    // Prepare HTTP get header
    std::stringstream ss;
    ss << "GET / HTTP/1.1\r\n"
       << "Host: apresolve.spotify.com\r\n"
       << "Accept: application/json\r\n"
       << "Connection: close\r\n"
       << "\r\n\r\n";

    std::string request = ss.str();

    // Send the request
    if (send(sockFd, request.c_str(), request.length(), 0) != (int)request.length())
    {
        CSPOT_LOG(error, "apresolve: can't send request");
        throw std::runtime_error("Resolve failed");
    }

    char cur;

    // skip read till json data
    while (read(sockFd, &cur, 1) > 0 && cur != '{');

    auto jsonData = std::string("{");

    // Read json structure
    while (read(sockFd, &cur, 1) > 0)
    {
        jsonData += cur;
    }

    close(sockFd);

    return jsonData;
}

std::string ApResolve::fetchFirstApAddress()
{
    if (configMan->apOverride != "")
    {
        return configMan->apOverride;
    }

    // Fetch json body
    auto jsonData = getApList();

    // Use cJSON to get first ap address
    auto root = cJSON_Parse(jsonData.c_str());
    auto apList = cJSON_GetObjectItemCaseSensitive(root, "ap_list");

    auto firstAp = cJSON_GetArrayItem(apList, 0);
    auto data = std::string(firstAp->valuestring);

    // release cjson memory
    cJSON_Delete(root);

    return data;
}
