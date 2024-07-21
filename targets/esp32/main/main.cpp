#include <MDNSService.h>
#include <arpa/inet.h>
#include <mbedtls/aes.h>
#include <stdio.h>
#include <string.h>
#include <atomic>
#include <memory>
#include <string>
#include "BellHTTPServer.h"
#include "BellLogger.h"  // for setDefaultLogger, AbstractLogger
#include "BellTask.h"
#include "civetweb.h"
#include "esp_event.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"

#include <CSpotContext.h>
#include <LoginBlob.h>
#include <SpircHandler.h>

#include <inttypes.h>
#include "BellTask.h"
#include "CircularBuffer.h"

#include "BellUtils.h"
#include "Logger.h"
#include "esp_log.h"

#define DEVICE_NAME CONFIG_CSPOT_DEVICE_NAME

#ifdef CONFIG_BELL_NOCODEC
#include "VSPlayer.h"
#include "VSinit.h"
#else
#include "EspPlayer.h"
#ifdef CONFIG_CSPOT_SINK_INTERNAL
#include <InternalAudioSink.h>
#endif
#ifdef CONFIG_CSPOT_SINK_AC101
#include <AC101AudioSink.h>
#endif
#ifdef CONFIG_CSPOT_SINK_ES8388
#include <ES8388AudioSink.h>
#endif
#ifdef CONFIG_CSPOT_SINK_ES9018
#include <ES9018AudioSink.h>
#endif
#ifdef CONFIG_CSPOT_SINK_PCM5102
#include <PCM5102AudioSink.h>
#endif
#ifdef CONFIG_CSPOT_SINK_TAS5711
#include <TAS5711AudioSink.h>
#endif
#endif

extern "C" {
void app_main(void);
}

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
    bell::MDNSService::registerService(
        blob->getDeviceName(), "_spotify-connect", "_tcp", "", serverPort,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});
    std::cout << "Waiting for spotify app to connect..." << std::endl;
  }
};

class CSpotTask : public bell::Task {
 private:
  std::unique_ptr<cspot::SpircHandler> handler;
#ifndef CONFIG_BELL_NOCODEC
  std::unique_ptr<AudioSink> audioSink;
#endif

 public:
  CSpotTask() : bell::Task("cspot", 8 * 1024, 0, 0) {
    startTask();
  }
  void runTask() {

    mdns_init();
    mdns_hostname_set("cspot");
#ifdef CONFIG_BELL_NOCODEC
    std::shared_ptr<VS1053_SINK> audioSink;
    audioSink = std::make_shared<VS1053_SINK>();
    initAudioSink(audioSink);
#else
#ifdef CONFIG_CSPOT_SINK_INTERNAL
    auto audioSink = std::make_unique<InternalAudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_AC101
    auto audioSink = std::make_unique<AC101AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_ES8388
    auto audioSink = std::make_unique<ES8388AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_ES9018
    auto audioSink = std::make_unique<ES9018AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_PCM5102
    auto audioSink = std::make_unique<PCM5102AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_TAS5711
    auto audioSink = std::make_unique<TAS5711>();
#endif
    audioSink->setParams(44100, 2, 16);
    audioSink->volumeChanged(160);
#endif

    auto loggedInSemaphore = std::make_shared<bell::WrappedSemaphore>(1);

    auto zeroconfServer = std::make_unique<ZeroconfAuthenticator>();
    std::atomic<bool> isRunning = true;

    zeroconfServer->onClose = [&isRunning]() {
      isRunning = false;
    };

    auto loginBlob = std::make_shared<cspot::LoginBlob>(DEVICE_NAME);
#ifdef CONFIG_CSPOT_LOGIN_PASS
    loginBlob->loadUserPass(CONFIG_CSPOT_LOGIN_USERNAME,
                            CONFIG_CSPOT_LOGIN_PASSWORD);
    loggedInSemaphore->give();

#else
    zeroconfServer->blob = loginBlob;
    zeroconfServer->onAuthSuccess = [loggedInSemaphore]() {
      loggedInSemaphore->give();
    };
    zeroconfServer->registerHandlers();
#endif
    loggedInSemaphore->wait();
    auto ctx = cspot::Context::createFromBlob(loginBlob);
    ctx->session->connectWithRandomAp();
    ctx->config.authData = ctx->session->authenticate(loginBlob);
    if (ctx->config.authData.size() > 0) {
      // when credentials file is set, then store reusable credentials

      // Start spirc task
      auto handler = std::make_shared<cspot::SpircHandler>(ctx);

      // Start handling mercury messages
      ctx->session->startTask();

      // Create a player, pass the handler
#ifndef CONFIG_BELL_NOCODEC
      auto player =
          std::make_shared<EspPlayer>(std::move(audioSink), std::move(handler));
#else
      auto player =
          std::make_shared<VSPlayer>(std::move(handler), std::move(audioSink));
#endif
      // If we wanted to handle multiple devices, we would halt this loop
      // when a new zeroconf login is requested, and reinitialize the session
      while (isRunning) {
        ctx->session->handlePacket();
      }

      // Never happens, but required for above case
      handler->disconnect();
      player->disconnect();
    }
  }
};

void init_spiffs() {
  esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE("SPIFFS", "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE("SPIFFS", "Failed to initialize SPIFFS (%s)",
                esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE("SPIFFS", "Failed to get SPIFFS partition information (%s)",
              esp_err_to_name(ret));
  } else {
    ESP_LOGE("SPIFFS", "Partition size: total: %d, used: %d", total, used);
  }
}

void app_main(void) {
  // statusLed = std::make_shared<ESPStatusLed>();
  // statusLed->setStatus(StatusLed::IDLE);

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  init_spiffs();

  // statusLed->setStatus(StatusLed::WIFI_CONNECTING);

  esp_wifi_set_ps(WIFI_PS_NONE);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect());

  // statusLed->setStatus(StatusLed::WIFI_CONNECTED);

  
  ESP_LOGI("MAIN", "Connected to AP, start spotify receiver");
  //auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 12*1024, NULL, 5, NULL, 1);
  /*auto taskHandle = */
  bell::setDefaultLogger();
  //bell::enableTimestampLogging();
  auto task = std::make_unique<CSpotTask>();
  vTaskSuspend(NULL);
}
