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
#include "Utils.h"
#include "Crypto.h"

#ifdef ESP_PLATFORM
#include "mdns.h"
#else
#include "dns_sd.h"
#include <unistd.h>
#include <fcntl.h>
#endif
#include <cstdlib>
#include <ctime>
#include "LoginBlob.h"

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
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