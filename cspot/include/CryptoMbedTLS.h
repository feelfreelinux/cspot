#ifndef CRYPTOMBEDTLS_H
#define CRYPTOMBEDTLS_H

#include <vector>
#include <string>

class CryptoMbedTLS {
public:
    // Base64
    std::vector<uint8_t> base64Decode(std::string &data);
    std::string base64Encode(std::vector<uint8_t> &data);

    // Sha1
    void sha1Update(const std::string &s);
    void sha1Update(std::vector<uint8_t> &vec);
    std::string sha1Final();
    std::vector<uint8_t> sha1FinalBytes();

    // HMAC SHA1
    std::vector<uint8_t> sha1HMAC(std::vector<uint8_t> &inputKey, std::vector<uint8_t> &message);

    // AES CTR
    std::vector<uint8_t> aesCTRXcrypt(std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data);

    // Diffie Hellman
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> privateKey;

    // PBKDF2
    std::vector<uint8_t> pbkdf2HmacSha1(std::vector<uint8_t> password, std::vector<uint8_t> salt, int iterations, int digestSize);

    void dhInit();
    std::vector<uint8_t> dhCalculateShared(std::vector<uint8_t> remoteKey);

    // Random stuff
    std::vector<uint8_t> generateVectorWithRandomData(size_t length);
};

#endif