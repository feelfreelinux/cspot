#pragma once

#include <cstddef>
#include <memory>
#include "Crypto.h"
#include "HTTPClient.h"
#include "WrappedSemaphore.h"

#include "Logger.h"
#include "Utils.h"
#include "CSpotContext.h"
#include "AccessKeyFetcher.h"


namespace cspot {

class CDNTrackStream {

 public:
  CDNTrackStream(std::shared_ptr<cspot::AccessKeyFetcher>);
  ~CDNTrackStream();

  enum class Status { INITIALIZING, HAS_DATA, HAS_URL, FAILED };

  struct TrackInfo {
    std::string name;
    std::string album;
    std::string artist;
    std::string imageUrl;
    int duration;
  };

  TrackInfo trackInfo;

  Status status;
  std::unique_ptr<bell::WrappedSemaphore> trackReady;

  void fetchFile(const std::vector<uint8_t>& trackId,
                 const std::vector<uint8_t>& audioKey);

  void fail();

  void openStream();

  size_t readBytes(uint8_t* dst, size_t bytes);

  size_t getPosition();

  size_t getSize();

  void seek(size_t position);

 private:
  const int OPUS_HEADER_SIZE = 8 * 1024;
  const int OPUS_FOOTER_PREFFERED = 1024 * 12; // 12K should be safe
  const int SEEK_MARGIN_SIZE = 1024 * 4;

  const int HTTP_BUFFER_SIZE = 1024 * 14;
  const int SPOTIFY_OPUS_HEADER = 167;

  // Used to store opus metadata, speeds up read
  std::vector<uint8_t> header = std::vector<uint8_t>(OPUS_HEADER_SIZE);
  std::vector<uint8_t> footer;

  // General purpose buffer to read data
  std::vector<uint8_t> httpBuffer = std::vector<uint8_t>(HTTP_BUFFER_SIZE);

  // AES IV for decrypting the audio stream
  const std::vector<uint8_t> audioAESIV = {0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb,
                                           0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64,
                                           0x3f, 0x63, 0x0d, 0x93};
  std::unique_ptr<Crypto> crypto;

  std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher;

  std::unique_ptr<bell::HTTPClient::Response> httpConnection;
  bool isConnected = false;

  size_t position = 0; // Spotify header size
  size_t totalFileSize = 0;
  size_t lastRequestPosition = 0;
  size_t lastRequestCapacity = 0;

  bool enableRequestMargin = false;

  std::string cdnUrl;
  std::vector<uint8_t> trackId;
  std::vector<uint8_t> audioKey;

  void decrypt(uint8_t* dst, size_t nbytes, size_t pos);
};
}  // namespace cspot