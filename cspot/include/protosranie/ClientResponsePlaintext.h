// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __CLIENT_RESPONSE_PLAINTEXTH
#define __CLIENT_RESPONSE_PLAINTEXTH
#include "LoginCryptoResponseUnion.h"
#include "PoWResponseUnion.h"
#include "CryptoResponseUnion.h"
class ClientResponsePlaintext {
public:
LoginCryptoResponseUnion login_crypto_response;
PoWResponseUnion pow_response;
CryptoResponseUnion crypto_response;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassClientResponsePlaintext;
};

#endif
