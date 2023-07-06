#include "AccessKeyFetcher.h"

#include <cstring>           // for strrchr
#include <initializer_list>  // for initializer_list
#include <map>               // for operator!=, operator==
#include <type_traits>       // for remove_extent_t
#include <vector>            // for vector

#include "BellLogger.h"    // for AbstractLogger
#include "CSpotContext.h"  // for Context
#include "HTTPClient.h"
#include "Logger.h"            // for CSPOT_LOG
#include "MercurySession.h"    // for MercurySession, MercurySession::Res...
#include "NanoPBExtensions.h"  // for bell::nanopb::encode...
#include "NanoPBHelper.h"      // for pbEncode and pbDecode
#include "Packet.h"            // for cspot
#include "TimeProvider.h"      // for TimeProvider
#include "Utils.h"             // for string_format

#ifdef BELL_ONLY_CJSON
#include "cJSON.h"
#else
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#endif

#include "protobuf/login5.pb.h"  // for LoginRequest

using namespace cspot;

static std::string CLIENT_ID =
    "65b708073fc0480ea92a077233ca87bd";  // Spotify web client's client id

static std::string SCOPES =
    "streaming,user-library-read,user-library-modify,user-top-read,user-read-"
    "recently-played";  // Required access scopes

AccessKeyFetcher::AccessKeyFetcher(std::shared_ptr<cspot::Context> ctx)
    : ctx(ctx) {}

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

  // Prepare a protobuf login request
  static LoginRequest loginRequest = LoginRequest_init_zero;
  static LoginResponse loginResponse = LoginResponse_init_zero;

  // Assign necessary request fields
  loginRequest.client_info.client_id.funcs.encode = &bell::nanopb::encodeString;
  loginRequest.client_info.client_id.arg = &CLIENT_ID;

  loginRequest.client_info.device_id.funcs.encode = &bell::nanopb::encodeString;
  loginRequest.client_info.device_id.arg = &ctx->config.deviceId;

  loginRequest.login_method.stored_credential.username.funcs.encode =
      &bell::nanopb::encodeString;
  loginRequest.login_method.stored_credential.username.arg =
      &ctx->config.username;

  // Set login method to stored credential
  loginRequest.which_login_method = LoginRequest_stored_credential_tag;
  loginRequest.login_method.stored_credential.data.funcs.encode =
      &bell::nanopb::encodeVector;
  loginRequest.login_method.stored_credential.data.arg = &ctx->config.authData;

  // Max retry of 3, can receive different hash cat types
  int retryCount = 3;
  bool success = false;

  do {
    auto encodedRequest = pbEncode(LoginRequest_fields, &loginRequest);
    CSPOT_LOG(info, "Access token expired, fetching new one... %d",
              encodedRequest.size());

    // Perform a login5 request, containing the encoded protobuf data
    auto response = bell::HTTPClient::post(
        "https://login5.spotify.com/v3/login",
        {{"Content-Type", "application/x-protobuf"}}, encodedRequest);

    auto responseBytes = response->bytes();

    // Deserialize the response
    pbDecode(loginResponse, LoginResponse_fields, responseBytes);

    if (loginResponse.which_response == LoginResponse_ok_tag) {
      // Successfully received an auth token
      CSPOT_LOG(info, "Access token sucessfully fetched");
      success = true;

      accessKey = std::string(loginResponse.response.ok.access_token);

      // Expire in ~30 minutes
      int expiresIn = 3600 / 2;

      if (loginResponse.response.ok.has_access_token_expires_in) {
        int expiresIn = loginResponse.response.ok.access_token_expires_in / 2;
      }

      this->expiresAt =
          ctx->timeProvider->getSyncedTimestamp() + (expiresIn * 1000);
    } else {
      CSPOT_LOG(error, "Failed to fetch access token");
    }

    // Free up allocated memory for response
    pb_release(LoginResponse_fields, &loginResponse);

    retryCount--;
  } while (retryCount >= 0 && !success);

  keyPending = false;
}
