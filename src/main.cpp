#include <iostream>
#include <string>
#include <memory>
#include <PlainConnection.h>
#include <Session.h>
#include "SpotifyTrack.h"
#include <SpircController.h>
#include <authentication.pb.h>
#include <MercuryManager.h>
#include <unistd.h>
#include <fstream>
extern "C"
{

#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c" // Enables Vorbis decoding.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "usage:\n";
        std::cout << "    cspot <username> <password>\n";
        return 1;
    }

    auto connection = std::make_shared<PlainConnection>();
    connection->connectToAp();

    auto session = std::make_unique<Session>();
    session->connect(connection);
    auto token = session->authenticate(std::string(argv[1]), std::string(argv[2]));

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(session->shanConn);
        mercuryManager->startTask();
        usleep(1000000);
        auto spircController = std::make_shared<SpircController>(mercuryManager, "fliperspotify");
        
        mercuryManager->waitForTaskToReturn();

    }
    return 0;
}