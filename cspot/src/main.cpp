#include <stdio.h>
#include <iostream>
#include <string.h>
#include <Connection.h>

int main()
{  
    auto connection = new Connection();
    connection->connectToAp();
    return 0;
}