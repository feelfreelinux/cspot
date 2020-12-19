#include "ApResolve.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <sstream>
#include <cJSON.h>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
// #include <Winsock.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

ApResolve::ApResolve() {}

std::string ApResolve::getApList()
{
    // hostname lookup
    struct hostent *host = gethostbyname("apresolve.spotify.com");
    struct sockaddr_in client;

    if ((host == NULL) || (host->h_addr == NULL))
    {
        printf("apresolve: DNS lookup error\n");
        throw std::runtime_error("Resolve failed");
    }

    // Prepare socket
    memset(&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(80);
    memcpy(&client.sin_addr, host->h_addr, host->h_length);

    // auto must be used because on windows socket() returns SOCKET, not int
    auto sockFd = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to spotify's server
    if (connect(sockFd, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        // todo: create a nice wrapper
#ifdef _WIN32
        closesocket(sockFd);
#else

        close(sockFd);
#endif
        printf("Could not connect to apresolve\n");
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
        printf("apresolve: can't send request\n");
        throw std::runtime_error("Resolve failed");
    }

    char cur;

    // skip read till json data
    while (recv(sockFd, &cur, 1, 0) > 0 && cur != '{')
        ;

    auto jsonData = std::string("{");

    // Read json structure
    while (recv(sockFd, &cur, 1, 0) > 0)
    {
        jsonData += cur;
    }

    return jsonData;
}

std::string ApResolve::fetchFirstApAddress()
{
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
