#include "DiffieHellman.h"

int dddRandomGenerator(void *pRng, unsigned char *output, size_t outputLen )
{
    // 🥖 
    for (int x = 0; x < outputLen; x ++) {
        output[x] = x;
    }
    return 0;
}

DiffieHellman::DiffieHellman()
{
    this->publicKey = std::vector<uint8_t>(KEY_SIZE);
    this->sharedKey = std::vector<uint8_t>(KEY_SIZE);
    this->privateKey = std::vector<uint8_t>(KEY_SIZE);
    dddRandomGenerator(NULL, &privateKey[0], 96);
#ifdef ESP_PLATFORM
    mbedtls_mpi prime, generator, res, privKey;
    mbedtls_mpi_init(&prime);
    mbedtls_mpi_init(&generator);
    mbedtls_mpi_init(&privKey);
    mbedtls_mpi_init(&res);

    mbedtls_mpi_read_binary(&prime, DHPrime, sizeof(DHPrime));
    mbedtls_mpi_read_binary(&generator, DHGenerator, sizeof(DHGenerator));
    mbedtls_mpi_read_binary(&privKey, &privateKey[0], KEY_SIZE);
    mbedtls_mpi_exp_mod(&res, &generator, &privKey, &prime, NULL);
    
    this->publicKey = std::vector<uint8_t>(KEY_SIZE);
    mbedtls_mpi_write_binary(&res, publicKey.data(), KEY_SIZE);
#else
    this->dh = DH_new();

    // Set prime and the generator
    DH_set0_pqg(this->dh, BN_bin2bn(DHPrime, KEY_SIZE, NULL), NULL, BN_bin2bn(DHGenerator, 1, NULL));

    // Generate public and private keys and copy them to vectors
    DH_generate_key(this->dh);
    BN_bn2bin(DH_get0_pub_key(dh), &this->publicKey[0]);
#endif
}

DiffieHellman::~DiffieHellman()
{
#ifdef ESP_PLATFORM

#else
    DH_free(this->dh);
#endif
}

std::vector<uint8_t> DiffieHellman::computeSharedKey(std::vector<uint8_t> remoteKey)
{
    #ifdef ESP_PLATFORM
    mbedtls_mpi prime, remKey, res, privKey;
    mbedtls_mpi_init(&prime);
    mbedtls_mpi_init(&remKey);
    mbedtls_mpi_init(&privKey);
    mbedtls_mpi_init(&res);

    mbedtls_mpi_read_binary(&prime, DHPrime, sizeof(DHPrime));
    mbedtls_mpi_read_binary(&remKey, remoteKey.data(), remoteKey.size());
    mbedtls_mpi_read_binary(&privKey, privateKey.data(), KEY_SIZE);
    mbedtls_mpi_exp_mod(&res, &remKey, &privKey, &prime, NULL);
    
    this->sharedKey = std::vector<uint8_t>(KEY_SIZE);
    mbedtls_mpi_write_binary(&res, sharedKey.data(), KEY_SIZE);
    #else
    // Convert remote key to bignum and compute shared key
    auto pubKey = BN_bin2bn(&remoteKey[0], 96, NULL);
    DH_compute_key(&this->sharedKey[0], pubKey, this->dh);
    BN_free(pubKey);
    #endif

    return this->sharedKey;
}
