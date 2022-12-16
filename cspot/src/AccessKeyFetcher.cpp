#include "AccessKeyFetcher.h"
#include "HTTPClient.h"
#include "Logger.h"
#include "Utils.h"

using namespace cspot;

AccessKeyFetcher::AccessKeyFetcher(
    std::shared_ptr<cspot::Context> ctx) {
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
        auto jsonBody = nlohmann::json::parse(res.parts[0].data());
        this->accessKey = jsonBody["accessToken"];
        int expiresIn = jsonBody["expiresIn"];
        expiresIn = expiresIn / 2; // Refresh token before it expires

        this->expiresAt=  timeProvider->getSyncedTimestamp() + (expiresIn * 1000);
        callback(jsonBody["accessToken"]);
      });
}