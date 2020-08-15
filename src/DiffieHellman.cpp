#include "DiffieHellman.h"

DiffieHellman::DiffieHellman()
{
    this->dh = DH_new();
    this->privateKey = std::vector<uint8_t>(KEY_SIZE);
    this->publicKey = std::vector<uint8_t>(KEY_SIZE);
    this->sharedKey = std::vector<uint8_t>(KEY_SIZE);

    // Set prime and the generator
    DH_set0_pqg(this->dh, BN_bin2bn(DHPrime, KEY_SIZE, NULL), NULL, BN_bin2bn(DHGenerator, 1, NULL));

    // Generate public and private keys and copy them to vectors
    DH_generate_key(this->dh);
    BN_bn2bin(DH_get0_priv_key(dh), &this->privateKey[0]);
    BN_bn2bin(DH_get0_pub_key(dh), &this->publicKey[0]);
}

std::vector<uint8_t> DiffieHellman::computeSharedKey(std::vector<uint8_t> remoteKey) {
    // Convert remote key to bignum and compute shared key
    auto pubKey = BN_bin2bn (&remoteKey[0], 96, NULL);
    DH_compute_key (&this->sharedKey[0], pubKey, this->dh);
    BN_free(pubKey);
    return this->sharedKey;
}
