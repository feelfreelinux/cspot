#ifndef ZEROCONFAUTHENTICATOR_H
#define ZEROCONFAUTHENTICATOR_H
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include "cjson/cJSON.h"
#include <fstream>

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

class ZeroconfAuthenticator {
private:
    void startServer();

public:
    ZeroconfAuthenticator();
    void listenForRequests();
};

#endif