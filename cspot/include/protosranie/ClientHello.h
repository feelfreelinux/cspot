// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __CLIENT_HELLOH
#define __CLIENT_HELLOH
#include <vector>
#include <optional>
#include "BuildInfo.h"
#include "LoginCryptoHelloUnion.h"
#include "Cryptosuite.h"
#include "FeatureSet.h"
class ClientHello {
public:
BuildInfo build_info;
LoginCryptoHelloUnion login_crypto_hello;
std::vector<Cryptosuite> cryptosuites_supported;
std::vector<uint8_t> client_nonce;
std::optional<std::vector<uint8_t>> padding;
std::optional<FeatureSet> feature_set;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassClientHello;
};

#endif
