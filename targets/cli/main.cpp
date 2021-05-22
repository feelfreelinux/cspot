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
#include "CommandLineArguments.h"

#include "ConfigJSON.h"
#include "CliFile.h"
#include "Logger.h"

std::shared_ptr<ConfigJSON> configMan;

int main(int argc, char **argv)
{
    try
    {

        std::string credentialsFileName = "authBlob.json";
        std::string configFileName = "config.json";

        std::ifstream blobFile(credentialsFileName);

        auto args = CommandLineArguments::parse(argc, argv);
        if (args->shouldShowHelp)
        {
            std::cout << "Usage: cspotcli [OPTION]...\n";
            std::cout << "Emulate a Spotify connect speaker.\n";
            std::cout << "\n";
            std::cout << "Run without any arguments to authenticate by using mDNS on the local network (open the spotify app and CSpot should appear as a device on the local network). \n";
            std::cout << "Alternatively you can specify a username and password to login with.";
            std::cout << "\n";
            std::cout << "-u, --username            your spotify username\n";
            std::cout << "-p, --password            your spotify password, note that if you use facebook login you can set a password in your account settings\n";
            std::cout << "\n";
            std::cout << "ddd 2021\n";
            return 0;
        }

        auto file = std::make_shared<CliFile>();
        configMan = std::make_shared<ConfigJSON>(configFileName, file);

        if(!configMan->load())
        {
          CSPOT_LOG(error, "Config error");
        }

        std::shared_ptr<LoginBlob> blob;
        // this means username/password login instead of mdns
        if (!args->username.empty())
        {
            blob = std::make_shared<LoginBlob>();
            blob->loadUserPass(args->username, args->password);
        }
        // Check if credential file exists
        else if (!blobFile.good())
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
            auto audioSink = std::make_shared<ALSAAudioSink>();
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
            auto audioSink = std::make_shared<PortAudioSink>();
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
    }
    catch (std::invalid_argument e)
    {
        std::cout << "Invalid options passed: " << e.what() << "\n";
        std::cout << "Pass --help for more informaton. \n";
        return 1; // we exit with an non-zero exit code
    }

    return 0;
}
