#include <iostream>
#include <string>
#include <memory>
#include <PlainConnection.h>
#include <Session.h>
#include <authentication.pb.h>

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
    session->authenticate(std::string(argv[1]), std::string(argv[2]));

    return 0;
}