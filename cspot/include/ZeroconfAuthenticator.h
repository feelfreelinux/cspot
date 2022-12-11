#ifndef ZEROCONFAUTHENTICATOR_H
#define ZEROCONFAUTHENTICATOR_H

#include <vector>
#include <functional>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <string>
#include <cstdlib>
#include "Utils.h"
#include "LoginBlob.h"
#include <map>
#include "Crypto.h"
#include "BellTask.h"
#include "ConstantParameters.h"


#ifdef ESP_PLATFORM
#include "mdns.h"
#elif defined(_WIN32)
#include "mdnssvc.h"
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
#ifdef _WIN32
    static struct mdnsd* service;
#endif
    int serverPort;
    bool authorized = false;
    std::unique_ptr<Crypto> crypto;

    authCallback gotBlobCallback;
    std::string name, deviceId;

public:
    ZeroconfAuthenticator(authCallback callback, int serverPort, std::string name, std::string deviceId);
    void registerHandlers();
    
    std::string buildJsonInfo();
    std::string getParameterFromUrlEncoded(std::string data, std::string param);
    void handleAddUser(std::map<std::string, std::string>& queryMap);
    void registerZeroconf();
};

#endif
