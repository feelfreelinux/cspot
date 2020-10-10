#ifndef ZEROCONFAUTHENTICATOR_H
#define ZEROCONFAUTHENTICATOR_H

#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <netinet/in.h>
#include <netdb.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <ctype.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include "Utils.h"
#include "LoginBlob.h"
#include "Crypto.h"
#include "ConstantParameters.h"


#ifdef ESP_PLATFORM
#include "mdns.h"
#else
#include "dns_sd.h"
#include <unistd.h>
#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define SERVER_PORT_MAX 65535 // Max usable tcp port
#define SERVER_PORT_MIN 1024 // 0-1024 services ports

class ZeroconfAuthenticator {
private:
    int serverPort;
    std::unique_ptr<Crypto> crypto;

    void startServer();
    std::string buildJsonInfo();
    std::shared_ptr<LoginBlob> handleAddUser(std::string user);
    std::string getParameterFromUrlEncoded(std::string data, std::string param);
    void registerZeroconf();
public:
    ZeroconfAuthenticator();
    std::shared_ptr<LoginBlob> listenForRequests();
};

#endif