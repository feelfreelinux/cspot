#include "CDNTrackStream.h"
#include <cstddef>
#include "HTTPClient.h"
#include "Logger.h"
#include "Utils.h"

using namespace cspot;

CDNTrackStream::CDNTrackStream(std::shared_ptr<cspot::Context> ctx) {
  this->ctx = ctx;
  this->accessKeyFetcher = std::make_shared<cspot::AccessKeyFetcher>(ctx);

  this->status = Status::INITIALIZING;
  this->trackReady = std::make_unique<bell::WrappedSemaphore>(0);
  this->crypto = std::make_unique<Crypto>();
}

CDNTrackStream::~CDNTrackStream() {}

void CDNTrackStream::fail() {
  this->status = Status::FAILED;
  this->trackReady->give();
}

void CDNTrackStream::fetchFile(const std::vector<uint8_t>& trackId,
                               const std::vector<uint8_t>& audioKey) {
  this->status = Status::HAS_DATA;
  this->trackId = trackId;
  this->audioKey = std::vector<uint8_t>(audioKey.begin() + 4, audioKey.end());

  accessKeyFetcher->getAccessKey([this, trackId, audioKey](std::string key) {
    CSPOT_LOG(info, "Received access key, fetching CDN URL...");

    std::string requestUrl = string_format(
        "https://api.spotify.com/v1/storage-resolve/files/audio/interactive/"
        "%s?alt=json&product=9",
        bytesToHexString(trackId).c_str());

    auto req = bell::HTTPClient::execute(bell::HTTPClient::HTTPRequest{
        .url = requestUrl, .headers = {{"Authorization", "Bearer " + key}}});

    std::string result = req->readToString();

    auto jsonResult = nlohmann::json::parse(result);
    std::string cdnUrl = jsonResult["cdnurl"][0];
    if (this->status != Status::FAILED) {

      this->cdnUrl = cdnUrl;
      this->status = Status::HAS_URL;
      CSPOT_LOG(info, "Received CDN URL, %s", cdnUrl.c_str());
      this->trackReady->give();
    }
  });
}

size_t CDNTrackStream::readBytes(uint8_t* dst, size_t bytes) {
  if (bytes % 16 != 0) {
    CSPOT_LOG(error, "Bytes to read must be a multiple of 16");
    return 0;
  }

  if (this->httpStream == nullptr) {
    CSPOT_LOG(info, "Opening HTTP stream to %s", this->cdnUrl.c_str());
    httpStream = bell::HTTPClient::execute({
        .url = this->cdnUrl,
    });
  }

  size_t toRead = bytes;

  while (toRead > 0) {
    size_t read = httpStream->read((char*)dst + (bytes - toRead), toRead);
    toRead -= read;
  }

  auto calculatedIV = bigNumAdd(audioAESIV, totalReadBytes / 16);

  this->crypto->aesCTRXcrypt(this->audioKey, calculatedIV, dst, bytes);
  totalReadBytes += bytes;

  return bytes;
}
