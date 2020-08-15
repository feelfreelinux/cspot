#include <stdio.h>
#include <iostream>
#include <string.h>
#include <PlainConnection.h>
#include <Session.h>
#include <authentication.pb.h>



int main()
{    
    auto connection = new PlainConnection();
    connection->connectToAp();

    auto session = new Session();
    session->connect(connection);

    ClientResponseEncrypted test = {};
    // test.login_credentials.username
    return 0;
}