#include <iostream>
#include <string>
#include <memory>
#include <PlainConnection.h>
#include <Session.h>
#include <authentication.pb.h>
#include <MercuryManager.h>

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
    if (token.size() > 0) {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_unique<MercuryManager>(session->shanConn);
        mercuryManager->startTask();

        mercuryManager->waitForTaskToReturn();
    }

    return 0;
}