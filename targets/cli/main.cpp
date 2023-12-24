#include <functional>   // for __base, function
#include <map>          // for operator!=, map, map<>::mapped_type
#include <stdexcept>    // for invalid_argument
#include <type_traits>  // for remove_extent_t
#include <vector>       // for vector
#include <iostream>
#include <fstream>
#include <sstream> // for std::osstringstream

#include "BellHTTPServer.h"       // for BellHTTPServer
#include "BellLogger.h"           // for setDefaultLogger, AbstractLogger
#include "CSpotContext.h"         // for Context, Context::ConfigState
#include "CliPlayer.h"            // for CliPlayer
#include "MDNSService.h"          // for MDNSService
#include "SpircHandler.h"         // for SpircHandler
#include "WrappedSemaphore.h"     // for WrappedSemaphore
#include "civetweb.h"             // for mg_header, mg_get_request_info
#include "nlohmann/json.hpp"      // for basic_json<>::object_t, basic_json
#include "nlohmann/json_fwd.hpp"  // for json
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <iostream>  // for operator<<, cout, ostream, basic_o...
#include <memory>    // for shared_ptr, make_shared, unique_ptr
#include <string>    // for string, char_traits, operator<

#include "CommandLineArguments.h"  // for CommandLineArguments
#include "Logger.h"                // for CSPOT_LOG
#include "LoginBlob.h"             // for LoginBlob

#if defined(CSPOT_ENABLE_ALSA_SINK)
#include "ALSAAudioSink.h"
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
#include "PortAudioSink.h"
#else
#include "NamedPipeAudioSink.h"  // for NamedPipeAudioSink
#endif

class ZeroconfAuthenticator {
 public:
  ZeroconfAuthenticator(){};
  ~ZeroconfAuthenticator(){};

  // Authenticator state
  int serverPort = 7864;

  // Use bell's HTTP server to handle the authentication, although anything can be used
  std::unique_ptr<bell::BellHTTPServer> server;
  std::shared_ptr<cspot::LoginBlob> blob;

  std::function<void()> onAuthSuccess;
  std::function<void()> onClose;

  void registerHandlers() {
    this->server = std::make_unique<bell::BellHTTPServer>(serverPort);

    server->registerGet("/spotify_info", [this](struct mg_connection* conn) {
      return this->server->makeJsonResponse(this->blob->buildZeroconfInfo());
    });

    server->registerGet("/close", [this](struct mg_connection* conn) {
      this->onClose();
      return this->server->makeEmptyResponse();
    });

    server->registerPost("/spotify_info", [this](struct mg_connection* conn) {
      nlohmann::json obj;
      // Prepare a success response for spotify
      obj["status"] = 101;
      obj["spotifyError"] = 0;
      obj["statusString"] = "ERROR-OK";

      std::string body = "";
      auto requestInfo = mg_get_request_info(conn);
      if (requestInfo->content_length > 0) {
        body.resize(requestInfo->content_length);
        mg_read(conn, body.data(), requestInfo->content_length);

        mg_header hd[10];
        int num = mg_split_form_urlencoded(body.data(), hd, 10);
        std::map<std::string, std::string> queryMap;

        // Parse the form data
        for (int i = 0; i < num; i++) {
          queryMap[hd[i].name] = hd[i].value;
        }

        CSPOT_LOG(info, "Received zeroauth POST data");

        // Pass user's credentials to the blob
        blob->loadZeroconfQuery(queryMap);

        // We have the blob, proceed to login
        onAuthSuccess();
      }

      return server->makeJsonResponse(obj.dump());
    });

    // Register mdns service, for spotify to find us
    MDNSService::registerService(
        blob->getDeviceName(), "_spotify-connect", "_tcp", "", serverPort,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});
    std::cout << "Waiting for spotify app to connect..." << std::endl;
  }
};

int main(int argc, char** argv) {
#ifdef _WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int WSerr = WSAStartup(wVersionRequested, &wsaData);
  if (WSerr != 0)
    exit(1);
#endif
  bell::setDefaultLogger();
  bell::enableTimestampLogging();
#ifdef CSPOT_ENABLE_ALSA_SINK
  auto audioSink = std::make_unique<ALSAAudioSink>();
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
  std::unique_ptr<AudioSink> audioSink = std::make_unique<PortAudioSink>();
#else
  auto audioSink = std::make_unique<NamedPipeAudioSink>();
#endif

  audioSink->setParams(44100, 2, 16);

  // Semaphore, main thread waits for this to be signaled before signing in
  auto loggedInSemaphore = std::make_shared<bell::WrappedSemaphore>(1);

  auto zeroconfServer = std::make_unique<ZeroconfAuthenticator>();
  std::atomic<bool> isRunning = true;

  zeroconfServer->onClose = [&isRunning]() {
    isRunning = false;
  };

  try {
    auto args = CommandLineArguments::parse(argc, argv);
    if (args->shouldShowHelp) {
      std::cout << "Usage: cspotcli [OPTION]...\n";
      std::cout << "Emulate a Spotify connect speaker.\n";
      std::cout << "\n";
      std::cout << "Run without any arguments to authenticate by using mDNS on "
                   "the local network (open the spotify app and CSpot should "
                   "appear as a device on the local network). \n";
      std::cout << "Alternatively you can specify a username and password to "
                   "login with.";
      std::cout << "\n";
      std::cout << "-u, --username            your spotify username\n";
      std::cout << "-p, --password            your spotify password, note that "
                   "if you use facebook login you can set a password in your "
                   "account settings\n";
      std::cout << "-c, --credentials         json file to store/load reusable credentials\n";
      std::cout << "-b, --bitrate             bitrate (320, 160, 96)\n";
      std::cout << "\n";
      std::cout << "ddd 2022\n";
      return 0;
    }

    // Create a login blob, pass a device name
    auto loginBlob = std::make_shared<cspot::LoginBlob>("CSpot player");

    // Login using Command line arguments
    if (!args->username.empty()) {
      loginBlob->loadUserPass(args->username, args->password);
      loggedInSemaphore->give();
    }
    // reusable credentials
    else if (!args->credentials.empty()) {
        std::ifstream file(args->credentials);
        std::ostringstream credentials;
        credentials << file.rdbuf();
        loginBlob->loadJson(credentials.str());
        loggedInSemaphore->give();
    }
    // ZeroconfAuthenticator
    else {
      zeroconfServer->blob = loginBlob;
      zeroconfServer->onAuthSuccess = [loggedInSemaphore]() {
        loggedInSemaphore->give();
      };
      zeroconfServer->registerHandlers();
    }

    // Wait for authentication to complete
    loggedInSemaphore->wait();
    auto ctx = cspot::Context::createFromBlob(loginBlob);

    // Apply preferences
    if (args->setBitrate) {
      ctx->config.audioFormat = args->bitrate;
    }

    CSPOT_LOG(info, "Creating player");
    ctx->session->connectWithRandomAp();
    ctx->config.authData = ctx->session->authenticate(loginBlob);

    // Auth successful
    if (ctx->config.authData.size() > 0) {
      // when credentials file is set, then store reusable credentials
      if (!args->credentials.empty()) {
          std::ofstream file(args->credentials);
          file << ctx->getCredentialsJson();
      }

      // Start spirc task
      auto handler = std::make_shared<cspot::SpircHandler>(ctx);

      // Start handling mercury messages
      ctx->session->startTask();

      // Create a player, pass the handler
      auto player = std::make_shared<CliPlayer>(std::move(audioSink), handler);

      // If we wanted to handle multiple devices, we would halt this loop
      // when a new zeroconf login is requested, and reinitialize the session
      while (isRunning) {
        ctx->session->handlePacket();
      }

      // Never happens, but required for above case
      handler->disconnect();
      player->disconnect();
    }

  } catch (std::invalid_argument e) {
    std::cout << "Invalid options passed: " << e.what() << "\n";
    std::cout << "Pass --help for more informaton. \n";
    return 1;  // we exit with an non-zero exit code
  }

  return 0;
}
