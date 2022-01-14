#include "LoginBlob.h"
#include "JSONObject.h"
#include "Logger.h"

LoginBlob::LoginBlob()
{
    this->crypto = std::make_unique<Crypto>();
}

std::vector<uint8_t> LoginBlob::decodeBlob(const std::vector<uint8_t> &blob, const std::vector<uint8_t> &sharedKey)
{
    // 0:16 - iv; 17:-20 - blob; -20:0 - checksum
    auto iv = std::vector<uint8_t>(blob.begin(), blob.begin() + 16);
    auto encrypted = std::vector<uint8_t>(blob.begin() + 16, blob.end() - 20);
    auto checksum = std::vector<uint8_t>(blob.end() - 20, blob.end());

    // baseKey = sha1(sharedKey) 0:16
    crypto->sha1Init();
    crypto->sha1Update(sharedKey);
    auto baseKey = crypto->sha1FinalBytes();
    baseKey = std::vector<uint8_t>(baseKey.begin(), baseKey.begin() + 16);

    auto checksumMessage = std::string("checksum");
    auto checksumKey = crypto->sha1HMAC(baseKey, std::vector<uint8_t>(checksumMessage.begin(), checksumMessage.end()));

    auto encryptionMessage = std::string("encryption");
    auto encryptionKey = crypto->sha1HMAC(baseKey, std::vector<uint8_t>(encryptionMessage.begin(), encryptionMessage.end()));

    auto mac = crypto->sha1HMAC(checksumKey, encrypted);

    // Check checksum
    if (mac != checksum)
    {
        CSPOT_LOG(error, "Mac doesn't match!" );
    }

    encryptionKey = std::vector<uint8_t>(encryptionKey.begin(), encryptionKey.begin() + 16);
    crypto->aesCTRXcrypt(encryptionKey, iv, encrypted.data(), encrypted.size());

    return encrypted;
}

uint32_t LoginBlob::readBlobInt(const std::vector<uint8_t> &data)
{
    auto lo = data[blobSkipPosition];
    if ((int)(lo & 0x80) == 0)
    {
        this->blobSkipPosition += 1;
        return lo;
    }

    auto hi = data[blobSkipPosition + 1];
    this->blobSkipPosition += 2;

    return (uint32_t)((lo & 0x7f) | (hi << 7));
}

std::vector<uint8_t> LoginBlob::decodeBlobSecondary(const std::vector<uint8_t> &blob, const std::string &username, const std::string &deviceId)
{
    auto encryptedString = std::string(blob.begin(), blob.end());
    auto blobData = crypto->base64Decode(encryptedString);

    crypto->sha1Init();
    crypto->sha1Update(std::vector<uint8_t>(deviceId.begin(), deviceId.end()));
    auto secret = crypto->sha1FinalBytes();
    auto pkBaseKey = crypto->pbkdf2HmacSha1(secret, std::vector<uint8_t>(username.begin(), username.end()), 256, 20);

    crypto->sha1Init();
    crypto->sha1Update(pkBaseKey);
    auto key = std::vector<uint8_t>({0x00, 0x00, 0x00, 0x14}); // len of base key
    auto baseKeyHashed = crypto->sha1FinalBytes();
    key.insert(key.begin(), baseKeyHashed.begin(), baseKeyHashed.end());

    crypto->aesECBdecrypt(key, blobData);

    auto l = blobData.size();

    for (int i = 0; i < l - 16; i++)
    {
        blobData[l - i - 1] ^= blobData[l - i - 17];
    }

    return blobData;
}

void LoginBlob::loadZeroconf(const std::vector<uint8_t> &blob, const std::vector<uint8_t> &sharedKey, const std::string &deviceId, const std::string &username)
{
    auto partDecoded = this->decodeBlob(blob, sharedKey);
    auto loginData = this->decodeBlobSecondary(partDecoded, username, deviceId);

    // Parse blob
    blobSkipPosition = 1;
    blobSkipPosition += readBlobInt(loginData);
    blobSkipPosition += 1;
    this->authType = readBlobInt(loginData);
    blobSkipPosition += 1;
    auto authSize = readBlobInt(loginData);
    this->username = username;
    this->authData = std::vector<uint8_t>(loginData.begin() + blobSkipPosition, loginData.begin() + blobSkipPosition + authSize);
}

void LoginBlob::loadUserPass(const std::string &username, const std::string &password)
{
    this->username = username;
    this->authData = std::vector<uint8_t>(password.begin(), password.end());
    this->authType = static_cast<uint32_t>(AuthenticationType_AUTHENTICATION_USER_PASS);
}

void LoginBlob::loadJson(const std::string &json)
{
    auto root = cJSON_Parse(json.c_str());
    auto authTypeObject = cJSON_GetObjectItemCaseSensitive(root, "authType");
    auto usernameObject = cJSON_GetObjectItemCaseSensitive(root, "username");
    auto authDataObject = cJSON_GetObjectItemCaseSensitive(root, "authData");

    auto authDataString = std::string(cJSON_GetStringValue(authDataObject));
    this->authData = crypto->base64Decode(authDataString);

    this->username = std::string(cJSON_GetStringValue(usernameObject));
    this->authType = cJSON_GetNumberValue(authTypeObject);

    cJSON_Delete(root);
}

std::string LoginBlob::toJson()
{
    bell::JSONObject obj;
    obj["authData"] = crypto->base64Encode(authData);
    obj["authType"] = this->authType;
    obj["username"] = this->username;
    
    return obj.toString();
}
