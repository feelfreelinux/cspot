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
#include "WrappedSemaphore.h"
#include "civetweb.h"
#include "esp_event.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"

#include <CSpotContext.h>
#include <LoginBlob.h>

#include <inttypes.h>

#include "BellUtils.h"
#include "Logger.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

//#define EXAMPLE_ESP_WIFI_SSID CONFIG_EXAMPLE_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS CONFIG_EXAMPLE_WIFI_PASSWORD
#define WIFI_AP_MAXIMUM_RETRY 5

#define DEVICE_NAME CONFIG_CSPOT_DEVICE_NAME

#ifdef CONFIG_BELL_NOCODEC
#include "VSPlayer.h"
#include "VSinit.h"
#else
#define TAG "INIT"
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

static int s_retry_num = 0;

class ZeroconfAuthenticator {
 public:
  ZeroconfAuthenticator(){};
  ~ZeroconfAuthenticator(){};

  // Authenticator state
  int serverPort = 7864;

  // Use bell's HTTP server to handle the authentication, although anything can be used
  std::unique_ptr<bell::BellHTTPServer> server;
  std::unique_ptr<bell::MDNSService> mdnsService;
  std::shared_ptr<cspot::LoginBlob> blob;

  std::function<void()> onAuthSuccess;
  std::function<void()> onClose;

  void registerMdnsService() {
    this->mdnsService = bell::MDNSService::registerService(
        blob->getDeviceName(), "_spotify-connect", "_tcp", "", serverPort,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});
  }

  void registerHandlers() {
    this->server = std::make_unique<bell::BellHTTPServer>(serverPort);

    server->registerGet("/spotify_info", [this](struct mg_connection* conn) {
      return this->server->makeJsonResponse(this->blob->buildZeroconfInfo());
    });

    server->registerGet("/close", [this](struct mg_connection* conn) {
      CSPOT_LOG(info, "Closing connection");
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
        if (1) {  //queryMap["userName"] != blob->getUserName()) {

          CSPOT_LOG(info, "Received zeroauth POST data");

          // Pass user's credentials to the blob
          blob->loadZeroconfQuery(queryMap);

          // We have the blob, proceed to login
#ifndef CONFIG_CSPOT_DISCOVERY_MODE_OPEN
          mdnsService->unregisterService();
#else 
          onClose();
#endif
          onAuthSuccess();
        } else {
          CSPOT_LOG(debug, "User already logged in, skipping auth");
        }
      }

      return server->makeJsonResponse(obj.dump());
    });

    // Register mdns service, for spotify to find us
    this->registerMdnsService();
    std::cout << "Waiting for spotify app to connect..." << std::endl;
  }
};

class CSpotTask : public bell::Task {
 private:
  //std::unique_ptr<cspot::DeviceStateHandler> handler;
#ifndef CONFIG_BELL_NOCODEC
  std::shared_ptr<AudioSink> audioSink;
#endif
  std::unique_ptr<ZeroconfAuthenticator> zeroconfServer;

 public:
  CSpotTask() : bell::Task("cspot", 16 * 1024, 0, 0) {
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
    auto audioSink = std::make_shared<InternalAudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_AC101
    auto audioSink = std::make_shared<AC101AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_ES8388
    auto audioSink = std::make_shared<ES8388AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_ES9018
    auto audioSink = std::make_shared<ES9018AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_PCM5102
    auto audioSink = std::make_shared<PCM5102AudioSink>();
#endif
#ifdef CONFIG_CSPOT_SINK_TAS5711
    auto audioSink = std::make_shared<TAS5711>();
#endif
    audioSink->setParams(44100, 2, 16);
    audioSink->volumeChanged(160);
#endif

    auto loggedInSemaphore = std::make_shared<bell::WrappedSemaphore>();

    std::atomic<bool> isRunningInDiscoveryMode = true;
    std::atomic<bool> isRunning = true;
    this->zeroconfServer = std::make_unique<ZeroconfAuthenticator>();

    auto loginBlob = std::make_shared<cspot::LoginBlob>(DEVICE_NAME);

    this->zeroconfServer->onClose = [this, &isRunning, &loginBlob]() {
      isRunning = false;
#ifndef CONFIG_CSPOT_DISCOVERY_MODE_OPEN
      CSPOT_LOG(info, "Waiting for spotify app to connect...");
      loginBlob = std::make_shared<cspot::LoginBlob>(DEVICE_NAME);
      this->zeroconfServer->blob = loginBlob;
      this->zeroconfServer->registerMdnsService();
#endif
    };

#ifdef CONFIG_CSPOT_LOGIN_PASS
    loginBlob->loadUserPass(CONFIG_CSPOT_LOGIN_USERNAME,
                            CONFIG_CSPOT_LOGIN_PASSWORD);
    loggedInSemaphore->give();

#else
    zeroconfServer->blob = loginBlob;
    zeroconfServer->onAuthSuccess = [loggedInSemaphore, &isRunning]() {
      if (!isRunning)
        loggedInSemaphore->give();
    };
    zeroconfServer->registerHandlers();
#endif
    while (true) {
      loggedInSemaphore->wait();
      isRunning = true;
      auto ctx = cspot::Context::createFromBlob(loginBlob);
    CSPOTConnecting:;
      try {
        ctx->session->connectWithRandomAp();
        ctx->config.authData = ctx->session->authenticate(loginBlob);
        if (ctx->config.authData.size() > 0) {
          // when credentials file is set, then store reusable credentials

          // Start device handler task
          auto handler = std::make_shared<cspot::DeviceStateHandler>(
              ctx, zeroconfServer->onClose);

          // Start handling mercury messages
          ctx->session->startTask();

          // Create a player, pass the handler
#ifndef CONFIG_BELL_NOCODEC
          auto player = std::make_shared<EspPlayer>(audioSink, handler);
#else
          auto player = std::make_shared<VSPlayer>(handler, audioSink);
#endif
          // If we wanted to handle multiple devices, we would halt this loop
          // when a new zeroconf login is requested, and reinitialize the session
          uint8_t taskCount = 0;
          while (isRunning) {
            ctx->session->handlePacket();
          }

          // Never happens, but required for above case
          handler->disconnect();
          player->disconnect();
        } else {
          std::cout << "Failed to authenticate" << std::endl;
        }
      } catch (std::exception& e) {
        std::cout << "Error while connecting " << e.what() << std::endl;
        goto CSPOTConnecting;
      }
    }
  }
};

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    s_retry_num++;
    ESP_LOGI(TAG, "retry to connect to the AP");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

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

void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
  wifi_config_t wifi_config = {};
  strcpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
         CONFIG_EXAMPLE_WIFI_SSID);
  strcpy(reinterpret_cast<char*>(wifi_config.sta.password),
         CONFIG_EXAMPLE_WIFI_PASSWORD);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
             CONFIG_EXAMPLE_WIFI_SSID, CONFIG_EXAMPLE_WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             CONFIG_EXAMPLE_WIFI_SSID, CONFIG_EXAMPLE_WIFI_SSID);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
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

  //init_spiffs();
  wifi_init_sta();

  // statusLed->setStatus(StatusLed::WIFI_CONNECTED);

  ESP_LOGI("MAIN", "Connected to AP, start spotify receiver");
  //auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 12*1024, NULL, 5, NULL, 1);
  /*auto taskHandle = */
  bell::setDefaultLogger();
  //bell::enableTimestampLogging();
  auto task = std::make_unique<CSpotTask>();
  vTaskSuspend(NULL);
}
