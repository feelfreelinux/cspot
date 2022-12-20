#include <atomic>
#include "AuthChallenges.h"
#include "BellHTTPServer.h"
#include "BellUtils.h"
#include "CSpotContext.h"
#include "CliPlayer.h"
#include "MDNSService.h"
#include "SpircHandler.h"
#include "civetweb.h"
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <ApResolve.h>
#include <PlainConnection.h>
#include <Session.h>
#include <inttypes.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>
#include "CommandLineArguments.h"
#include "LoginBlob.h"
#include "URLParser.h"

#ifdef CSPOT_ENABLE_ALSA_SINK
#include "ALSAAudioSink.h"
#elif defined(CSPOT_ENABLE_PORTAUDIO_SINK)
#include "PortAudioSink.h"
#else
#include "NamedPipeAudioSink.h"
#endif

#include "Logger.h"

bool createdFromZeroconf = false;

int main(int argc, char** argv) {
#ifdef _WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int WSerr = WSAStartup(wVersionRequested, &wsaData);
  if (WSerr != 0)
    exit(1);
#endif
  bell::setDefaultLogger();

  std::atomic<bool> gotBlob = false;
  std::atomic<bool> running = true;
  auto server = std::make_unique<bell::BellHTTPServer>(7864);

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
      std::cout << "-b, --bitrate             bitrate (320, 160, 96)\n";
      std::cout << "\n";
      std::cout << "ddd 2022\n";
      return 0;
    }

    if (args->setBitrate) {
      // configMan->format = args->bitrate;
    }
    auto blob = std::make_shared<LoginBlob>("CSpot player");

    // Login using Command line arguments
    if (!args->username.empty()) {
      blob->loadUserPass(args->username, args->password);
      gotBlob = true;
    }
    // ZeroconfAuthenticator
    else {
      server->registerGet(
          "/spotify_info", [&server, blob](struct mg_connection* conn) {
            return server->makeJsonResponse(blob->buildZeroconfInfo());
          });
      server->registerGet(
          "/disconnect", [&server, &running, blob](struct mg_connection* conn) {
            running = false;
            return server->makeJsonResponse("ok");
          });
      server->registerPost("/spotify_info", [&server, blob, &gotBlob](
                                                struct mg_connection* conn) {
        nlohmann::json obj;
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

          for (int i = 0; i < num; i++) {
            queryMap[hd[i].name] = hd[i].value;
          }

          blob->loadZeroconfQuery(queryMap);
          gotBlob = true;
        }

        return server->makeJsonResponse(obj.dump());
      });

      MDNSService::registerService(
          blob->getDeviceName(), "_spotify-connect", "_tcp", "", 7864,
          {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});
      std::cout << "Waiting for spotify app to connect..." << std::endl;
    }

  waitForBlob:
    while (!gotBlob) {
      BELL_SLEEP_MS(100);
    }
    running = true;

    if (gotBlob) {
      auto ctx = cspot::Context::createFromBlob(blob);
      CSPOT_LOG(info, "Creating player");
      ctx->session->connectWithRandomAp();
      auto token = ctx->session->authenticate(blob);

      // Auth successful
      if (token.size() > 0) {
        ctx->session->startTask();
        auto handler = std::make_shared<cspot::SpircHandler>(ctx);
        handler->subscribeToMercury();
        auto player = std::make_shared<CliPlayer>(handler);

        while (running) {
          ctx->session->handlePacket();
        }

        handler->disconnect();
        player->disconnect();

        running = false;
      }
    }

    gotBlob = false;
    goto waitForBlob;

  } catch (std::invalid_argument e) {
    std::cout << "Invalid options passed: " << e.what() << "\n";
    std::cout << "Pass --help for more informaton. \n";
    return 1;  // we exit with an non-zero exit code
  }

  return 0;
}
