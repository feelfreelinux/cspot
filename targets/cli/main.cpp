#include "BellUtils.h"
#include "CSpotContext.h"
#include "CliPlayer.h"
#include "HTTPClient2.h"
#include "SpircHandler.h"
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

#include "CliFile.h"
#include "ConfigJSON.h"
#include "Logger.h"

std::shared_ptr<ConfigJSON> configMan;
std::shared_ptr<CliFile> file;

std::string credentialsFileName = "authBlob.json";
std::string configFileName = "config.json";
bool createdFromZeroconf = false;

int main(int argc, char** argv) {
#ifdef _WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int WSerr = WSAStartup(wVersionRequested, &wsaData);
  if (WSerr != 0)
    exit(1);
#endif

  // auto req = bell2::HTTPClient::get("https://audio-fa.scdn.co/audio/962244253eb77af60ee5a47ddc1936a8277a1565?1671393092_6OUmJufSbYnL1c7n-gV1jHi9YrWOhD8Rn6YnEzhZUhw=", {
  //   bell2::HTTPClient::RangeHeader::range(0, 128)
  // });
  // std::cout << req.body() << std::endl;
  // return 0;
  try {
    bell::setDefaultLogger();

    std::ifstream blobFile(credentialsFileName);

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
      std::cout << "ddd 2021\n";
      return 0;
    }

    file = std::make_shared<CliFile>();
    configMan = std::make_shared<ConfigJSON>(configFileName, file);
    if (args->setBitrate) {
      configMan->format = args->bitrate;
    }

    if (!configMan->load()) {
      CSPOT_LOG(error, "Config error");
    }

    auto createPlayerCallback = [](std::shared_ptr<LoginBlob> blob) {
      CSPOT_LOG(info, "Creating player");

      auto ctx = cspot::Context::create();
      ctx->session->connectWithRandomAp();
      auto token = ctx->session->authenticate(blob);

      // Auth successful
      if (token.size() > 0) {
        ctx->session->startTask();
        auto handler = std::make_shared<cspot::SpircHandler>(ctx);
        handler->subscribeToMercury();

        auto player = std::make_shared<CliPlayer>(handler);
        ctx->session->handlePacket();
      }
    };

    // Blob file
    std::shared_ptr<LoginBlob> blob;
    std::string jsonData;

    bool read_status = file->readFile(credentialsFileName, jsonData);

    // Login using Command line arguments
    if (!args->username.empty()) {
      blob = std::make_shared<LoginBlob>();
      blob->loadUserPass(args->username, args->password);
      createPlayerCallback(blob);
    }
    // Login using Blob
    else if (jsonData.length() > 0 && read_status) {
      blob = std::make_shared<LoginBlob>();
      blob->loadJson(jsonData);
      createPlayerCallback(blob);
    }
    // ZeroconfAuthenticator
    else {}

    while (true)
      ;
  } catch (std::invalid_argument e) {
    std::cout << "Invalid options passed: " << e.what() << "\n";
    std::cout << "Pass --help for more informaton. \n";
    return 1;  // we exit with an non-zero exit code
  }

  return 0;
}
