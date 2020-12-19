#ifndef ZEROCONFAUTHENTICATOR_H
#define ZEROCONFAUTHENTICATOR_H

#include <vector>

#include <string>
#include <stdlib.h>
#include <sstream>
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
#elif _WIN32
#include <Windns.h>
#else
#include "dns_sd.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/stat.h>

#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define SERVER_PORT_MAX 65535 // Max usable tcp port
#define SERVER_PORT_MIN 1024  // 0-1024 services ports

class ZeroconfAuthenticator
{
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
