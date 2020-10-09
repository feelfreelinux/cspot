#ifndef CRYPTOMBEDTLS_H
#define CRYPTOMBEDTLS_H

#include <vector>
#include <string>
#include <memory>

#include <mbedtls/base64.h>
#include <mbedtls/bignum.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>


#define DH_KEY_SIZE 96

static unsigned char DHPrime[] = {
    /* Well-known Group 1, 768-bit prime */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9,
    0x0f, 0xda, 0xa2, 0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6,
    0x62, 0x8b, 0x80, 0xdc, 0x1c, 0xd1, 0x29, 0x02, 0x4e,
    0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6,
    0x3b, 0x13, 0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e,
    0x34, 0x04, 0xdd, 0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a,
    0x43, 0x1b, 0x30, 0x2b, 0x0a, 0x6d, 0xf2, 0x5f, 0x14,
    0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45,
    0xe4, 0x85, 0xb5, 0x76, 0x62, 0x5e, 0x7e, 0xc6, 0xf4,
    0x4c, 0x42, 0xe9, 0xa6, 0x3a, 0x36, 0x20, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static unsigned char DHGenerator[1] = {2};

class CryptoMbedTLS {
private:
    mbedtls_md_context_t sha1Context;
public:
    CryptoMbedTLS();
    ~CryptoMbedTLS();
    // Base64
    std::vector<uint8_t> base64Decode(const std::string& data);
    std::string base64Encode(const std::vector<uint8_t>& data);

    // Sha1
    void sha1Init();
    void sha1Update(const std::string& s);
    void sha1Update(const std::vector<uint8_t>& vec);
    std::string sha1Final();
    std::vector<uint8_t> sha1FinalBytes();

    // HMAC SHA1
    std::vector<uint8_t> sha1HMAC(const std::vector<uint8_t>& inputKey, const std::vector<uint8_t>& message);

    // AES CTR
    void aesCTRXcrypt(const std::vector<uint8_t>& key, std::vector<uint8_t>& iv, std::vector<uint8_t>& data);
    
    // AES ECB
    void aesECBdecrypt(const std::vector<uint8_t>& key, std::vector<uint8_t>& data);

    // Diffie Hellman
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> privateKey;
    void dhInit();
    std::vector<uint8_t> dhCalculateShared(const std::vector<uint8_t>& remoteKey);

    // PBKDF2
    std::vector<uint8_t> pbkdf2HmacSha1(const std::vector<uint8_t>& password, const std::vector<uint8_t>& salt, int iterations, int digestSize);

    // Random stuff
    std::vector<uint8_t> generateVectorWithRandomData(size_t length);
};

#endif