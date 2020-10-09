#include <iostream>
#include <string>
#include <memory>
#include <PlainConnection.h>
#include <Session.h>
#include "SpotifyTrack.h"
#include <SpircController.h>
#include <MercuryManager.h>
#include <NamedPipeAudioSink.h>
#include <ApResolve.h>
#include "ZeroconfAuthenticator.h"
#include "Crypto.h"
#include "LoginBlob.h"

#include <inttypes.h>

int main(int argc, char **argv)
{

    auto authenticator = std::make_shared<ZeroconfAuthenticator>();
    auto blob = authenticator->listenForRequests();

    auto apResolver = std::make_unique<ApResolve>();
    auto connection = std::make_shared<PlainConnection>();

    auto apAddr = apResolver->fetchFirstApAddress();
    connection->connectToAp(apAddr);

    auto session = std::make_unique<Session>();
    session->connect(connection);
    auto token = session->authenticate(blob);

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(session->shanConn);
        mercuryManager->startTask();
        auto audioSink = std::make_shared<NamedPipeAudioSink>();
        auto spircController = std::make_shared<SpircController>(mercuryManager, std::string(argv[1]), audioSink);

        mercuryManager->handleQueue();
    }

    while(true);

    return 0;
}