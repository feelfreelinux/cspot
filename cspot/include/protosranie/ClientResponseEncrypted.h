// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __CLIENT_RESPONSE_ENCRYPTEDH
#define __CLIENT_RESPONSE_ENCRYPTEDH
#include <optional>
#include "LoginCredentials.h"
#include "SystemInfo.h"
class ClientResponseEncrypted {
public:
LoginCredentials login_credentials;
SystemInfo system_info;
std::optional<std::string> version_string;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassClientResponseEncrypted;
};

#endif
