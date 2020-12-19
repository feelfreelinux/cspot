#include <string>
#include <streambuf>
#include <SpircController.h>
#include <Session.h>
#include <PlainConnection.h>
#include <MercuryManager.h>
#include <memory>
#include <vector>
#include <iostream>
#include <inttypes.h>
#include <fstream>
#include <ApResolve.h>
#include "ZeroconfAuthenticator.h"
#include "SpotifyTrack.h"
#include "NamedPipeAudioSink.h"
#include "LoginBlob.h"
#include "PortAudioSink.h"
#include "ALSAAudioSink.h"
#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    WSADATA wsaData;
    int iResult;
    u_long iMode = 0;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        std::cout << "Error at WSAStartup()";
        return 1;
    }
#endif
    std::string credentialsFileName = "authBlob.json";
    std::ifstream blobFile(credentialsFileName);

    std::shared_ptr<LoginBlob> blob;

    // Check if credential file exists
    if (!blobFile.good())
    {
        // Start zeroauth if not authenticated yet
        auto authenticator = std::make_shared<ZeroconfAuthenticator>();
        blob = authenticator->listenForRequests();

        // Store blob to file
        std::ofstream blobJsonFile(credentialsFileName);
        blobJsonFile << blob->toJson();
        blobJsonFile.close();
    }
    else
    {
        // Load blob from json and reuse it
        std::string jsonData((std::istreambuf_iterator<char>(blobFile)),
                             std::istreambuf_iterator<char>());

        blob = std::make_shared<LoginBlob>();
        // Assemble blob from json
        blob->loadJson(jsonData);
    }

    auto session = std::make_unique<Session>();
    session->connectWithRandomAp();
    auto token = session->authenticate(blob);

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(std::move(session));
        mercuryManager->startTask();

#ifdef CSPOT_ENABLE_ALSA_SINK
        auto audioSink = std::make_shared<NamedPipeAudioSink>();
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
        auto audioSink = std::make_shared<NamedPipeAudioSink>();
#else
        auto audioSink = std::make_shared<NamedPipeAudioSink>();
#endif
        auto spircController = std::make_shared<SpircController>(mercuryManager, blob->username, audioSink);
        mercuryManager->reconnectedCallback = [spircController]() {
            return spircController->subscribe();
        };

        mercuryManager->handleQueue();
    }

    while (true)
        ;

    return 0;
}
