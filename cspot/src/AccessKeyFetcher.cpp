#include "AccessKeyFetcher.h"
#include <cstring>
#include "Logger.h"
#include "Utils.h"

using namespace cspot;

AccessKeyFetcher::AccessKeyFetcher(std::shared_ptr<cspot::Context> ctx) {
  this->ctx = ctx;
}

AccessKeyFetcher::~AccessKeyFetcher() {}

bool AccessKeyFetcher::isExpired() {
  if (accessKey.empty()) {
    return true;
  }

  if (ctx->timeProvider->getSyncedTimestamp() > expiresAt) {
    return true;
  }

  return false;
}

void AccessKeyFetcher::getAccessKey(AccessKeyFetcher::Callback callback) {
  if (!isExpired()) {
    return callback(accessKey);
  }

  CSPOT_LOG(info, "Access token expired, fetching new one...");

  std::string url =
      string_format("hm://keymaster/token/authenticated?client_id=%s&scope=%s",
                    CLIENT_ID.c_str(), SCOPES.c_str());
  auto timeProvider = this->ctx->timeProvider;

  ctx->session->execute(
      MercurySession::RequestType::GET, url,
      [this, timeProvider, callback](MercurySession::Response& res) {
        std::cout << "Response size: " << res.parts.size() << std::endl;
        char* accessKeyJson = (char*)res.parts[0].data();
        auto accessJSON = std::string(accessKeyJson, strrchr(accessKeyJson, '}') - accessKeyJson + 1);
        std::cout << accessJSON << std::endl;
        auto jsonBody = nlohmann::json::parse(accessJSON);
        this->accessKey = jsonBody["accessToken"];
        int expiresIn = jsonBody["expiresIn"];
        expiresIn = expiresIn / 2;  // Refresh token before it expires

        this->expiresAt =
            timeProvider->getSyncedTimestamp() + (expiresIn * 1000);
        callback(jsonBody["accessToken"]);
      });
}
