// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __LOGIN_CREDENTIALSH
#define __LOGIN_CREDENTIALSH
#include <optional>
#include <vector>
#include "AuthenticationType.h"
class LoginCredentials {
public:
std::optional<std::string> username;
AuthenticationType typ;
std::optional<std::vector<uint8_t>> auth_data;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassLoginCredentials;
};

#endif
