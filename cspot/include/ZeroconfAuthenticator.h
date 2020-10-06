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
#include "cJSON.h"
#include "ConstantParameters.h"
#include <fstream>
#include "DiffieHellman.h"
#include "Base64.h"
#include "dns_sd.h"
#include <cstdlib>
#include <ctime>

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define SERVER_PORT_MAX 65535 // Max usable tcp port
#define SERVER_PORT_MIN 1024 // 0-1024 services ports

class ZeroconfAuthenticator {
private:
    int serverPort;
    std::unique_ptr<DiffieHellman> localKeys;

    void startServer();
    std::string buildJsonInfo();
    void handleAddUser(std::string user);
    std::string getParameterFromUrlEncoded(std::string data, std::string param);
    void registerZeroconf();
public:
    ZeroconfAuthenticator();
    void listenForRequests();
};

#endif