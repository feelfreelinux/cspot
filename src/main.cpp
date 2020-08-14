#include <stdio.h>
#include <iostream>
#include <string.h>
#include <PlainConnection.h>

int main()
{  
    auto connection = new PlainConnection();
    connection->connectToAp();
    return 0;
}