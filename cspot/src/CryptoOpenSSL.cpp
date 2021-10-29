#include "CryptoOpenSSL.h"
namespace
{
    struct BIOFreeAll
    {
        void operator()(BIO *p) { BIO_free_all(p); }
    };
} // namespace

CryptoOpenSSL::CryptoOpenSSL()
{
    // OpenSSL init
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();
    this->publicKey = std::vector<uint8_t>(DH_KEY_SIZE);
    this->privateKey = generateVectorWithRandomData(DH_KEY_SIZE);
}

CryptoOpenSSL::~CryptoOpenSSL()
{
    if (this->dhContext != nullptr)
    {
        DH_free(this->dhContext);
    }
}

std::vector<uint8_t> CryptoOpenSSL::base64Decode(const std::string& data)
{
    // base64 in openssl is an absolute mess lmao
    std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
    BIO *source = BIO_new_mem_buf(data.c_str(), -1); // read-only source
    BIO_push(b64.get(), source);
    const int maxlen = data.size() / 4 * 3 + 1;
    std::vector<uint8_t> decoded(maxlen);
    const int len = BIO_read(b64.get(), decoded.data(), maxlen);
    decoded.resize(len);
    return decoded;
}

std::string CryptoOpenSSL::base64Encode(const std::vector<uint8_t>& data)
{
    // base64 in openssl is an absolute mess lmao x 2
    std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));

    // No newline mode, put all the data into sink
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
    BIO *sink = BIO_new(BIO_s_mem());
    BIO_push(b64.get(), sink);
    BIO_write(b64.get(), data.data(), data.size());
    BIO_flush(b64.get());
    const char *encoded;
    const long len = BIO_get_mem_data(sink, &encoded);
    return std::string(encoded, len);
}

// Sha1
void CryptoOpenSSL::sha1Init()
{
    SHA1_Init(&sha1Context);
}

void CryptoOpenSSL::sha1Update(const std::string& s)
{
    sha1Update(std::vector<uint8_t>(s.begin(), s.end()));
}
void CryptoOpenSSL::sha1Update(const std::vector<uint8_t>& vec)
{
    SHA1_Update(&sha1Context, vec.data(), vec.size());
}

std::vector<uint8_t> CryptoOpenSSL::sha1FinalBytes()
{
    std::vector<uint8_t> digest(20); // 20 is 160 bits
    SHA1_Final(digest.data(), &sha1Context);
    return digest;
}

std::string CryptoOpenSSL::sha1Final()
{
    auto digest = sha1FinalBytes();
    return std::string(digest.begin(), digest.end());
}

// HMAC SHA1
std::vector<uint8_t> CryptoOpenSSL::sha1HMAC(const std::vector<uint8_t>& inputKey, const std::vector<uint8_t>& message)
{
    std::vector<uint8_t> digest(20); // 20 is 160 bits
    auto hmacContext = HMAC_CTX_new();
    HMAC_Init_ex(hmacContext, inputKey.data(), inputKey.size(), EVP_sha1(), NULL);
    HMAC_Update(hmacContext, message.data(), message.size());

    unsigned int resLen = 0;
    HMAC_Final(hmacContext, digest.data(), &resLen);

    HMAC_CTX_free(hmacContext);

    return digest;
}

// AES CTR
void CryptoOpenSSL::aesCTRXcrypt(const std::vector<uint8_t>& key, std::vector<uint8_t>& iv, std::vector<uint8_t> &data)
{
    // Prepare AES_KEY
    auto cryptoKey = AES_KEY();
    AES_set_encrypt_key(key.data(), 128, &cryptoKey);

    // Needed for openssl internal cache
    unsigned char ecountBuf[16] = {0};
    unsigned int offsetInBlock = 0;

    CRYPTO_ctr128_encrypt(
        data.data(),
        data.data(),
        data.size(),
        &cryptoKey,
        iv.data(),
        ecountBuf,
        &offsetInBlock,
        block128_f(AES_encrypt));
}

void CryptoOpenSSL::aesECBdecrypt(const std::vector<uint8_t>& key, std::vector<uint8_t>& data)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    int len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_192_ecb(), NULL, key.data(), NULL);
    EVP_DecryptUpdate(ctx, data.data(), &len, data.data(), data.size());
    EVP_DecryptFinal_ex(ctx, data.data() + len, &len);

    EVP_CIPHER_CTX_free(ctx);
}

// PBKDF2
std::vector<uint8_t> CryptoOpenSSL::pbkdf2HmacSha1(const std::vector<uint8_t>& password, const std::vector<uint8_t>& salt, int iterations, int digestSize)
{
    std::vector<uint8_t> digest(digestSize);

    // Generate PKDF2 digest
    PKCS5_PBKDF2_HMAC_SHA1((const char *)password.data(), password.size(),
                           (const unsigned char *)salt.data(), salt.size(), iterations,
                           digestSize, digest.data());
    return digest;
}

void CryptoOpenSSL::dhInit()
{
    // Free old context
    if (this->dhContext != nullptr)
    {
        DH_free(this->dhContext);
    }
    this->dhContext = DH_new();

    // Set prime and the generator
    DH_set0_pqg(this->dhContext, BN_bin2bn(DHPrime, DH_KEY_SIZE, NULL), NULL, BN_bin2bn(DHGenerator, 1, NULL));

    // Generate public and private keys and copy them to vectors
    DH_generate_key(this->dhContext);
    BN_bn2bin(DH_get0_pub_key(dhContext), this->publicKey.data());
}

std::vector<uint8_t> CryptoOpenSSL::dhCalculateShared(const std::vector<uint8_t>& remoteKey)
{
    auto sharedKey = std::vector<uint8_t>(DH_KEY_SIZE);
    // Convert remote key to bignum and compute shared key
    auto pubKey = BN_bin2bn(&remoteKey[0], DH_KEY_SIZE, NULL);
    DH_compute_key(sharedKey.data(), pubKey, this->dhContext);
    BN_free(pubKey);
    return sharedKey;
}

// Random stuff
std::vector<uint8_t> CryptoOpenSSL::generateVectorWithRandomData(size_t length)
{
    std::vector<uint8_t> randomVec(length);
    if(RAND_bytes(randomVec.data(), length) == 0)
    {
    }
    return randomVec;
}
