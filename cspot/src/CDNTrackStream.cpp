#include "CDNTrackStream.h"

using namespace cspot;

CDNTrackStream::CDNTrackStream(
    std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher) {
  this->accessKeyFetcher = accessKeyFetcher;
  this->status = Status::INITIALIZING;
  this->trackReady = std::make_unique<bell::WrappedSemaphore>(5);
  this->crypto = std::make_unique<Crypto>();
}

CDNTrackStream::~CDNTrackStream() {
}

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

    auto req = bell::HTTPClient::get(
        requestUrl,
        {bell::HTTPClient::ValueHeader({"Authorization", "Bearer " + key})});

    std::string_view result = req->body();

#ifdef BELL_ONLY_CJSON
    cJSON* jsonResult = cJSON_Parse(result.data());
    std::string cdnUrl = cJSON_GetArrayItem(cJSON_GetObjectItem(jsonResult, "cdnurl"), 0)->valuestring;
    cJSON_Delete(jsonResult);
#else
    auto jsonResult = nlohmann::json::parse(result);
    std::string cdnUrl = jsonResult["cdnurl"][0];
#endif
    if (this->status != Status::FAILED) {

      this->cdnUrl = cdnUrl;
      this->status = Status::HAS_URL;
      CSPOT_LOG(info, "Received CDN URL, %s", cdnUrl.c_str());

      this->openStream();
      this->trackReady->give();
    }
  });
}

size_t CDNTrackStream::getPosition() {
  return this->position;
}

void CDNTrackStream::seek(size_t newPos) {
  this->enableRequestMargin = true;
  this->position = newPos;
}

void CDNTrackStream::openStream() {
  CSPOT_LOG(info, "Opening HTTP stream to %s", this->cdnUrl.c_str());

  // Open connection, read first 128 bytes
  this->httpConnection = bell::HTTPClient::get(
      this->cdnUrl,
      {bell::HTTPClient::RangeHeader::range(0, OPUS_HEADER_SIZE - 1)});

  this->httpConnection->stream().read((char*)header.data(), OPUS_HEADER_SIZE);
  this->totalFileSize =
      this->httpConnection->totalLength() - SPOTIFY_OPUS_HEADER;

  this->decrypt(header.data(), OPUS_HEADER_SIZE, 0);

  // Location must be dividable by 16
  size_t footerStartLocation =
      (this->totalFileSize - OPUS_FOOTER_PREFFERED + SPOTIFY_OPUS_HEADER) -
      (this->totalFileSize - OPUS_FOOTER_PREFFERED + SPOTIFY_OPUS_HEADER) % 16;

  this->footer = std::vector<uint8_t>(
      this->totalFileSize - footerStartLocation + SPOTIFY_OPUS_HEADER);
  this->httpConnection->get(
      cdnUrl, {bell::HTTPClient::RangeHeader::last(footer.size())});

  this->httpConnection->stream().read((char*)footer.data(),
                                      this->footer.size());

  this->decrypt(footer.data(), footer.size(), footerStartLocation);
  CSPOT_LOG(info, "Header and footer bytes received");
  this->position = 0;
  this->lastRequestPosition = 0;
  this->lastRequestCapacity = 0;
  this->isConnected = true;
}

size_t CDNTrackStream::readBytes(uint8_t* dst, size_t bytes) {
  size_t offsetPosition = position + SPOTIFY_OPUS_HEADER;
  size_t actualFileSize = this->totalFileSize + SPOTIFY_OPUS_HEADER;

  if (position + bytes >= this->totalFileSize) {
    return 0;
  }

  // // Opus tries to read header, use prefetched data
  if (offsetPosition < OPUS_HEADER_SIZE &&
      bytes + offsetPosition <= OPUS_HEADER_SIZE) {
    memcpy(dst, this->header.data() + offsetPosition, bytes);
    position += bytes;
    return bytes;
  }

  // // Opus tries to read footer, use prefetched data
  if (offsetPosition >= (actualFileSize - this->footer.size())) {
    size_t toReadBytes = bytes;

    if ((position + bytes) > this->totalFileSize) {
      // Tries to read outside of bounds, truncate
      toReadBytes = this->totalFileSize - position;
    }

    size_t footerOffset =
        offsetPosition - (actualFileSize - this->footer.size());
    memcpy(dst, this->footer.data() + footerOffset, toReadBytes);

    position += toReadBytes;
    return toReadBytes;
  }

  // Data not in the headers. Make sense of whats going on.
  // Position in bounds :)
  if (offsetPosition >= this->lastRequestPosition &&
      offsetPosition < this->lastRequestPosition + this->lastRequestCapacity) {
    size_t toRead = bytes;

    if ((toRead + offsetPosition) >
        this->lastRequestPosition + lastRequestCapacity) {
      toRead = this->lastRequestPosition + lastRequestCapacity - offsetPosition;
    }

    memcpy(dst, this->httpBuffer.data() + offsetPosition - lastRequestPosition,
           toRead);
    position += toRead;

    return toRead;
  } else {
    size_t requestPosition = (offsetPosition) - ((offsetPosition) % 16);
    if (this->enableRequestMargin && requestPosition > SEEK_MARGIN_SIZE) {
      requestPosition = (offsetPosition - SEEK_MARGIN_SIZE) -
                        ((offsetPosition - SEEK_MARGIN_SIZE) % 16);
      this->enableRequestMargin = false;
    }

    this->httpConnection->get(
        cdnUrl, {bell::HTTPClient::RangeHeader::range(
                    requestPosition, requestPosition + HTTP_BUFFER_SIZE - 1)});
    this->lastRequestPosition = requestPosition;
    this->lastRequestCapacity = this->httpConnection->contentLength();

    this->httpConnection->stream().read((char*)this->httpBuffer.data(),
                                        lastRequestCapacity);
    this->decrypt(this->httpBuffer.data(), lastRequestCapacity,

                  this->lastRequestPosition);

    return readBytes(dst, bytes);
  }

  return bytes;
}

size_t CDNTrackStream::getSize() {
  return this->totalFileSize;
}

void CDNTrackStream::decrypt(uint8_t* dst, size_t nbytes, size_t pos) {
  auto calculatedIV = bigNumAdd(audioAESIV, pos / 16);

  this->crypto->aesCTRXcrypt(this->audioKey, calculatedIV, dst, nbytes);
}
