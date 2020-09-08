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
#include "fstream"
#include <inttypes.h>
#include <ChunkedAudioStream.h>

static unsigned char DHPrime2[] = {
    /* Well-known Group 1, 768-bit prime */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9,
    0x0f, 0xda, 0xa2, 0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6,
    0x62, 0x8b, 0x80, 0xdc, 0x1c, 0xd1, 0x29, 0x02, 0x4e,
    0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6,
    0x3b, 0x13, 0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e,
    0x34, 0x04, 0xdd, 0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a,
    0x43, 0x1b, 0x30, 0x2b, 0x0a, 0x6d, 0xf2, 0x5f, 0x14,
    0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45,
    0xe4, 0x85, 0xb5, 0x76, 0x62, 0x5e, 0x7e, 0xc6, 0xf4,
    0x4c, 0x42, 0xe9, 0xa6, 0x3a, 0x36, 0x20, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned char DHGenerator2[1] = {2};

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
        auto spircController = std::make_shared<SpircController>(mercuryManager, std::string(argv[1]));

        mercuryManager->handleQueue();
    }

    while(true);

    return 0;
}