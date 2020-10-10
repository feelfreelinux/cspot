#include "CryptoMbedTLS.h"

CryptoMbedTLS::CryptoMbedTLS()
{
}

CryptoMbedTLS::~CryptoMbedTLS()
{
}

std::vector<uint8_t> CryptoMbedTLS::base64Decode(const std::string &data)
{
    // Calculate max decode length
    size_t requiredSize;

    mbedtls_base64_encode(nullptr, 0, &requiredSize, (unsigned char *)data.c_str(), data.size());

    std::vector<uint8_t> output(requiredSize);
    size_t outputLen = 0;
    mbedtls_base64_decode(output.data(), requiredSize, &outputLen, (unsigned char *)data.c_str(), data.size());

    return std::vector<uint8_t>(output.begin(), output.begin() + outputLen);
}

std::string CryptoMbedTLS::base64Encode(const std::vector<uint8_t> &data)
{
    // Calculate max output length
    size_t requiredSize;
    mbedtls_base64_encode(nullptr, 0, &requiredSize, data.data(), data.size());

    std::vector<uint8_t> output(requiredSize);
    size_t outputLen = 0;

    mbedtls_base64_encode(output.data(), requiredSize, &outputLen, data.data(), data.size());

    return std::string(output.begin(), output.begin() + outputLen);
}

// Sha1
void CryptoMbedTLS::sha1Init()
{
    // Init mbedtls md context, pick sha1
    mbedtls_md_init(&sha1Context);
    mbedtls_md_setup(&sha1Context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    mbedtls_md_starts(&sha1Context);
}

void CryptoMbedTLS::sha1Update(const std::string &s)
{
    sha1Update(std::vector<uint8_t>(s.begin(), s.end()));
}
void CryptoMbedTLS::sha1Update(const std::vector<uint8_t> &vec)
{
    mbedtls_md_update(&sha1Context, vec.data(), vec.size());
}

std::vector<uint8_t> CryptoMbedTLS::sha1FinalBytes()
{
    std::vector<uint8_t> digest(20); // SHA1 digest size

    mbedtls_md_finish(&sha1Context, digest.data());
    mbedtls_md_free(&sha1Context);

    return digest;
}

std::string CryptoMbedTLS::sha1Final()
{
    auto digest = sha1FinalBytes();
    return std::string(digest.begin(), digest.end());
}

// HMAC SHA1
std::vector<uint8_t> CryptoMbedTLS::sha1HMAC(const std::vector<uint8_t> &inputKey, const std::vector<uint8_t> &message)
{
    std::vector<uint8_t> digest(20); // SHA1 digest size

    sha1Init();
    mbedtls_md_hmac_starts(&sha1Context, inputKey.data(), inputKey.size());
    mbedtls_md_hmac_update(&sha1Context, message.data(), message.size());
    mbedtls_md_hmac_finish(&sha1Context, digest.data());
    mbedtls_md_free(&sha1Context);

    return digest;
}

// AES CTR
void CryptoMbedTLS::aesCTRXcrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data)
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    // needed for internal cache
    size_t off = 0;
    unsigned char streamBlock[16] = {0};

    // set IV
    mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8);

    // Perform decrypt
    mbedtls_aes_crypt_ctr(&ctx,
                          data.size(),
                          &off,
                          iv.data(),
                          streamBlock,
                          data.data(),
                          data.data());
    mbedtls_aes_free(&ctx);
}

void CryptoMbedTLS::aesECBdecrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &data)
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    // Set 192bit key
    mbedtls_aes_setkey_dec(&ctx, key.data(), key.size() * 8);

    // Mbedtls's decrypt only works on 16 byte blocks
    for (unsigned int x = 0; x < data.size() / 16; x++)
    {
        // Perform decrypt
        mbedtls_aes_crypt_ecb(&ctx,
                              MBEDTLS_AES_DECRYPT,
                              data.data() + (x * 16),
                              data.data() + (x * 16));
    }

    mbedtls_aes_free(&ctx);
}

// PBKDF2
std::vector<uint8_t> CryptoMbedTLS::pbkdf2HmacSha1(const std::vector<uint8_t> &password, const std::vector<uint8_t> &salt, int iterations, int digestSize)
{
    auto digest = std::vector<uint8_t>(digestSize);

    // Init sha context
    sha1Init();
    mbedtls_pkcs5_pbkdf2_hmac(&sha1Context, password.data(), password.size(), salt.data(), salt.size(), iterations, digestSize, digest.data());

    // Free sha context
    mbedtls_md_free(&sha1Context);

    return digest;
}

void CryptoMbedTLS::dhInit()
{
    privateKey = generateVectorWithRandomData(DH_KEY_SIZE);

    // initialize big num
    mbedtls_mpi prime, generator, res, privKey;
    mbedtls_mpi_init(&prime);
    mbedtls_mpi_init(&generator);
    mbedtls_mpi_init(&privKey);
    mbedtls_mpi_init(&res);

    // Read bin into big num mpi
    mbedtls_mpi_read_binary(&prime, DHPrime, sizeof(DHPrime));
    mbedtls_mpi_read_binary(&generator, DHGenerator, sizeof(DHGenerator));
    mbedtls_mpi_read_binary(&privKey, privateKey.data(), DH_KEY_SIZE);

    // perform diffie hellman G^X mod P
    mbedtls_mpi_exp_mod(&res, &generator, &privKey, &prime, NULL);

    // Write generated public key to vector
    this->publicKey = std::vector<uint8_t>(DH_KEY_SIZE);
    mbedtls_mpi_write_binary(&res, publicKey.data(), DH_KEY_SIZE);

    // Release memory
    mbedtls_mpi_free(&prime);
    mbedtls_mpi_free(&generator);
    mbedtls_mpi_free(&privKey);
    //mbedtls_mpi_free(&res);
}

std::vector<uint8_t> CryptoMbedTLS::dhCalculateShared(const std::vector<uint8_t> &remoteKey)
{
    // initialize big num
    mbedtls_mpi prime, remKey, res, privKey;
    mbedtls_mpi_init(&prime);
    mbedtls_mpi_init(&remKey);
    mbedtls_mpi_init(&privKey);
    mbedtls_mpi_init(&res);

    // Read bin into big num mpi
    mbedtls_mpi_read_binary(&prime, DHPrime, sizeof(DHPrime));
    mbedtls_mpi_read_binary(&remKey, remoteKey.data(), remoteKey.size());
    mbedtls_mpi_read_binary(&privKey, privateKey.data(), DH_KEY_SIZE);

    // perform diffie hellman (G^Y)^X mod P (for shared secret)
    mbedtls_mpi_exp_mod(&res, &remKey, &privKey, &prime, NULL);

    auto sharedKey = std::vector<uint8_t>(DH_KEY_SIZE);
    mbedtls_mpi_write_binary(&res, sharedKey.data(), DH_KEY_SIZE);

    // Release memory
    mbedtls_mpi_free(&prime);
    mbedtls_mpi_free(&remKey);
    mbedtls_mpi_free(&privKey);
    mbedtls_mpi_free(&res);

    return sharedKey;
}

// Random stuff
std::vector<uint8_t> CryptoMbedTLS::generateVectorWithRandomData(size_t length)
{
    std::vector<uint8_t> randomVector(length);
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbg;
    // Personification string
    const char *pers = "cspotGen";

    // init entropy and random num generator
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);

    // Seed the generator
    mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                          (const unsigned char *)pers,
                          7);

    // Generate random bytes
    mbedtls_ctr_drbg_random(&ctrDrbg, randomVector.data(), length);

    // Release memory
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbg);

    return randomVector;
}