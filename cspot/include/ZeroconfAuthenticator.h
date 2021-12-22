#ifndef ZEROCONFAUTHENTICATOR_H
#define ZEROCONFAUTHENTICATOR_H

#include <vector>
#include <unistd.h>
#include <string>
#include <BaseHTTPServer.h>
#include <cstdlib>
#include "Utils.h"
#include "LoginBlob.h"
#include "Crypto.h"
#include "Task.h"
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

typedef std::function<void(std::shared_ptr<LoginBlob>)> authCallback;

class ZeroconfAuthenticator {
private:
    int serverPort;
    bool authorized = false;
    std::unique_ptr<Crypto> crypto;
    std::shared_ptr<bell::BaseHTTPServer> server;
    authCallback gotBlobCallback;
    void startServer();
    std::string buildJsonInfo();
    void handleAddUser(std::map<std::string, std::string>& queryMap);
    void registerZeroconf();
    std::string getParameterFromUrlEncoded(std::string data, std::string param);
public:
    ZeroconfAuthenticator(authCallback callback, std::shared_ptr<bell::BaseHTTPServer> httpServer);
    void registerHandlers();
};

#endif
