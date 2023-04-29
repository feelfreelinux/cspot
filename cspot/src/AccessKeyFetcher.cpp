#include "AccessKeyFetcher.h"

#include <cstring>           // for strrchr
#include <initializer_list>  // for initializer_list
#include <map>               // for operator!=, operator==
#include <type_traits>       // for remove_extent_t
#include <vector>            // for vector

#include "BellLogger.h"      // for AbstractLogger
#include "CSpotContext.h"    // for Context
#include "Logger.h"          // for CSPOT_LOG
#include "MercurySession.h"  // for MercurySession, MercurySession::Res...
#include "Packet.h"          // for cspot
#include "TimeProvider.h"    // for TimeProvider
#include "Utils.h"           // for string_format
#include "WrappedSemaphore.h"
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json

using namespace cspot;

static std::string CLIENT_ID =
    "65b708073fc0480ea92a077233ca87bd";  // Spotify web client's client id

static std::string SCOPES =
    "streaming,user-library-read,user-library-modify,user-top-read,user-read-"
    "recently-played";  // Required access scopes

AccessKeyFetcher::AccessKeyFetcher(std::shared_ptr<cspot::Context> ctx)
    : ctx(ctx) {
  this->updateSemaphore = std::make_shared<bell::WrappedSemaphore>();
}

bool AccessKeyFetcher::isExpired() {
  if (accessKey.empty()) {
    return true;
  }

  if (ctx->timeProvider->getSyncedTimestamp() > expiresAt) {
    return true;
  }

  return false;
}

std::string AccessKeyFetcher::getAccessKey() {
  if (!isExpired()) {
    return accessKey;
  }

  updateAccessKey();

  return accessKey;
}

void AccessKeyFetcher::updateAccessKey() {
  if (keyPending) {
    // Already pending refresh request
    return;
  }

  keyPending = true;

  CSPOT_LOG(info, "Access token expired, fetching new one...");

  std::string url =
      string_format("hm://keymaster/token/authenticated?client_id=%s&scope=%s",
                    CLIENT_ID.c_str(), SCOPES.c_str());
  auto timeProvider = this->ctx->timeProvider;

  ctx->session->execute(
      MercurySession::RequestType::GET, url,
      [this, timeProvider](MercurySession::Response& res) {
        if (res.fail)
          return;
        auto accessJSON = std::string((char*)res.parts[0].data(), res.parts[0].size());
#ifdef BELL_ONLY_CJSON
        cJSON* jsonBody = cJSON_Parse(accessJSON.c_str());
        this->accessKey =
            cJSON_GetObjectItem(jsonBody, "accessToken")->valuestring;
        int expiresIn = cJSON_GetObjectItem(jsonBody, "expiresIn")->valueint;
#else
        auto jsonBody = nlohmann::json::parse(accessJSON);
        this->accessKey = jsonBody["accessToken"];
        int expiresIn = jsonBody["expiresIn"];
#endif
        expiresIn = expiresIn / 2;  // Refresh token before it expires

        this->expiresAt =
            timeProvider->getSyncedTimestamp() + (expiresIn * 1000);
#ifdef BELL_ONLY_CJSON
        callback(cJSON_GetObjectItem(jsonBody, "accessToken")->valuestring);
        cJSON_Delete(jsonBody);
#else
        accessKey = (jsonBody["accessToken"]);
        updateSemaphore->give();
#endif
      });

  updateSemaphore->twait(5000);
}
