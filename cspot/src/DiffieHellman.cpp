#include "DiffieHellman.h"

DiffieHellman::DiffieHellman()
{
    this->publicKey = std::vector<uint8_t>(KEY_SIZE);
    this->sharedKey = std::vector<uint8_t>(KEY_SIZE);
#ifdef USE_MBEDTLS
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy, NULL, 0);
    mbedtls_mpi prime, generator;
    mbedtls_mpi_init(&prime);
    mbedtls_mpi_init(&generator);
    mbedtls_mpi_read_binary(&prime, DHPrime, sizeof(DHPrime));
    mbedtls_mpi_read_binary(&generator, DHGenerator, sizeof(DHGenerator));
    mbedtls_dhm_init(&dhm);
    mbedtls_dhm_set_group(&dhm, &prime, &generator);
    mbedtls_dhm_make_public(&dhm, KEY_SIZE, &publicKey[0], KEY_SIZE,
                            mbedtls_ctr_drbg_random, &ctrDrbg);
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
#ifdef USE_MBEDTLS
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_dhm_free(&dhm);
#else
    DH_free(this->dh);
#endif
}

std::vector<uint8_t> DiffieHellman::computeSharedKey(std::vector<uint8_t> remoteKey)
{
    #ifdef USE_MBEDTLS
    mbedtls_dhm_read_public(&dhm, &remoteKey[0], KEY_SIZE);
    size_t keySize = KEY_SIZE;
    mbedtls_dhm_calc_secret(&dhm, &this->sharedKey[0], KEY_SIZE, &keySize, 
        mbedtls_ctr_drbg_random, &ctrDrbg);
    #else
    // Convert remote key to bignum and compute shared key
    auto pubKey = BN_bin2bn(&remoteKey[0], 96, NULL);
    DH_compute_key(&this->sharedKey[0], pubKey, this->dh);
    BN_free(pubKey);
    #endif

    return this->sharedKey;
}
