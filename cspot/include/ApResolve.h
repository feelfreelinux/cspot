#ifndef APRESOLVE_H
#define APRESOLVE_H
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
#include <cJSON.h>
#include <fstream>

class ApResolve {
private:
    std::string getApList();

public:
    ApResolve();
    std::string fetchFirstApAddress();
};

#endif