#pragma once

#include <memory>
#include "AccessKeyFetcher.h"
#include "CSpotContext.h"
#include "HTTPClient.h"
#include "Crypto.h"
#include "WrappedSemaphore.h"
#include "core_http_client.h"
#include "plaintext_posix.h"

struct NetworkContext {
  PlaintextParams_t *pParams;
};

namespace cspot {

class CDNTrackStream {

 public:
  CDNTrackStream(std::shared_ptr<cspot::Context>);
  ~CDNTrackStream();

  enum class Status { INITIALIZING, HAS_DATA, HAS_URL, FAILED };

  Status status;
  std::unique_ptr<bell::WrappedSemaphore> trackReady;

  void fetchFile(const std::vector<uint8_t>& trackId,
                 const std::vector<uint8_t>& audioKey);

  void fail();

  size_t readBytes(uint8_t* dst, size_t bytes);

 private:
  // AES IV for decrypting the audio stream
  const std::vector<uint8_t> audioAESIV = {0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb,
                                           0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64,
                                           0x3f, 0x63, 0x0d, 0x93};
  std::unique_ptr<Crypto> crypto;

  std::shared_ptr<cspot::Context> ctx;
  std::shared_ptr<cspot::AccessKeyFetcher> accessKeyFetcher;

  bell::HTTPResponse_t httpStream = nullptr;
  std::vector<uint8_t> spotifyHeader = std::vector<uint8_t>(167);
  size_t posOffset = 167;

  size_t totalReadBytes = 0;

  std::string cdnUrl;
  std::vector<uint8_t> trackId;
  std::vector<uint8_t> audioKey;

  NetworkContext_t networkContext;
  std::vector<uint8_t> buffer = std::vector<uint8_t>(2048);
};
}  // namespace cspot