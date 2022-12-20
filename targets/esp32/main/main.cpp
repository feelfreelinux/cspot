#include <MDNSService.h>
#include <arpa/inet.h>
#include <mbedtls/aes.h>
#include <stdio.h>
#include <string.h>
#include <atomic>
#include <memory>
#include <string>
#include "BellHTTPServer.h"
#include "BellTask.h"
#include "civetweb.h"
#include "esp_event.h"
#include "esp_log.h"
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
#include "ES8311AudioSink.h"
#include "ESPStatusLed.h"
#include "Logger.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#define DEVICE_NAME CONFIG_CSPOT_DEVICE_NAME

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

static const char* TAG = "cspot";

std::shared_ptr<ESPStatusLed> statusLed;
std::string credentialsFileName = "/spiffs/authBlob.json";
bool createdFromZeroconf = false;

extern "C" {
void app_main(void);
}

class CSpotPlayer : public bell::Task {
 private:
  std::shared_ptr<cspot::SpircHandler> handler;
  std::unique_ptr<ES8311AudioSink> audioSink;
  std::unique_ptr<bell::CircularBuffer> circularBuffer;
  std::atomic<bool> isPaused;

 public:
  CSpotPlayer(std::shared_ptr<cspot::SpircHandler> handler)
      : bell::Task("cspot", 8 * 1024, 0, 0) {
    this->handler = handler;
    this->audioSink = std::make_unique<ES8311AudioSink>();
    this->audioSink->setParams(44100, 2, 16);
    this->audioSink->volumeChanged(160);

    this->circularBuffer =
        std::make_unique<bell::CircularBuffer>(1024 * 128 * 8);

    this->handler->getTrackPlayer()->setDataCallback(
        [this](uint8_t* data, size_t bytes) { this->feedData(data, bytes); });
    this->isPaused = false;

    this->handler->setEventHandler(
        [this](std::unique_ptr<cspot::SpircHandler::Event> event) {
          switch (event->eventType) {
            case cspot::SpircHandler::EventType::PLAY_PAUSE:
              this->isPaused = std::get<bool>(event->data);
              break;
            case cspot::SpircHandler::EventType::FLUSH:
              this->circularBuffer->emptyBuffer();
              break;
            case cspot::SpircHandler::EventType::SEEK:
              this->circularBuffer->emptyBuffer();
              break;
            case cspot::SpircHandler::EventType::PLAYBACK_START:
              this->circularBuffer->emptyBuffer();
            default:
              break;
          }
        });
    startTask();
  }

  void feedData(uint8_t* data, size_t len) {
    size_t toWrite = len;

    while (toWrite > 0) {
      size_t written =
          this->circularBuffer->write(data + (len - toWrite), toWrite);
      if (written == 0) {
        BELL_SLEEP_MS(10);
      }

      toWrite -= written;
    }
  }

  void runTask() {
    std::vector<uint8_t> outBuf = std::vector<uint8_t>(1024);

    while (true) {
      if (!this->isPaused) {
        size_t read = this->circularBuffer->read(outBuf.data(), outBuf.size());
        this->audioSink->feedPCMFrames(outBuf.data(), read);

        if (read == 0) {
          BELL_SLEEP_MS(100);
        }
      } else {
        BELL_SLEEP_MS(100);
      }
    }
  }
};

class CSpotTask : public bell::Task {
 public:
  CSpotTask() : bell::Task("cspot", 32 * 1024, 0, 1) { startTask(); }

  void runTask() {
    mdns_init();
    mdns_hostname_set("cspot");
    std::atomic<bool> gotBlob = false;

    auto blob = std::make_shared<LoginBlob>(DEVICE_NAME);

    auto server = std::make_unique<bell::BellHTTPServer>(8080);
    server->registerGet(
        "/spotify_info", [&server, blob](struct mg_connection* conn) {
          return server->makeJsonResponse(blob->buildZeroconfInfo());
        });
    server->registerPost(
        "/spotify_info", [&server, blob, &gotBlob](struct mg_connection* conn) {
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

    bell::MDNSService::registerService(
        blob->getDeviceName(), "_spotify-connect", "_tcp", "", 8080,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});

    while (!gotBlob) {
      BELL_SLEEP_MS(1000);
      BELL_LOG(info, "cspot", "Waiting for spotify app to connect...");
    }

    BELL_LOG(info, "cspot", "Got blob!");
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
        auto player = std::make_shared<CSpotPlayer>(handler);

        while (true) {
          ctx->session->handlePacket();
        }

        handler->disconnect();
        //   player->disconnect();
      }
    }
  }
};

static void cspotTask(void* pvParameters) {

  // #ifdef CONFIG_CSPOT_SINK_INTERNAL
  //     auto audioSink = std::make_shared<InternalAudioSink>();
  // #endif
  // #ifdef CONFIG_CSPOT_SINK_AC101
  //     auto audioSink = std::make_shared<AC101AudioSink>();
  // #endif
  // #ifdef CONFIG_CSPOT_SINK_ES8388
  //     auto audioSink = std::make_shared<ES8388AudioSink>();
  // #endif
  // #ifdef CONFIG_CSPOT_SINK_ES9018
  //     auto audioSink = std::make_shared<ES9018AudioSink>();
  // #endif
  // #ifdef CONFIG_CSPOT_SINK_PCM5102
  //     auto audioSink = std::make_shared<PCM5102AudioSink>();
  // #endif
  // #ifdef CONFIG_CSPOT_SINK_TAS5711
  //     auto audioSink = std::make_shared<TAS5711AudioSink>();
  // #endif

  //     // Config file
  //     file = std::make_shared<ESPFile>();
  //     configMan = std::make_shared<ConfigJSON>("/spiffs/config.json", file);

  //     if (!configMan->load()) {
  //         CSPOT_LOG(error, "Config error");
  //     }

  //     configMan->deviceName = DEVICE_NAME;
  //     #ifdef CONFIG_CSPOT_QUALITY_96
  //     configMan->format = AudioFormat_OGG_VORBIS_96;
  //     #endif
  //     #ifdef CONFIG_CSPOT_QUALITY_160
  //     configMan->format = AudioFormat_OGG_VORBIS_160;
  //     #endif
  //     #ifdef CONFIG_CSPOT_QUALITY_320
  //     configMan->format = AudioFormat_OGG_VORBIS_320;
  //     #endif

  //     auto createPlayerCallback = [audioSink](std::shared_ptr<LoginBlob> blob) {

  //         //        heap_trace_start(HEAP_TRACE_LEAKS);
  //         //        esp_dump_per_task_heap_info();

  //         CSPOT_LOG(info, "Creating player");
  //         statusLed->setStatus(StatusLed::SPOT_INITIALIZING);

  //         auto session = std::make_unique<Session>();
  //         session->connectWithRandomAp();
  //         auto token = session->authenticate(blob);

  //         // Auth successful
  //         if (token.size() > 0)
  //         {
  //             if (createdFromZeroconf) {
  //                 file->writeFile(credentialsFileName, blob->toJson());
  //             }

  //             statusLed->setStatus(StatusLed::SPOT_READY);

  //             mercuryManager = std::make_shared<MercuryManager>(std::move(session));
  //             mercuryManager->startTask();

  //             spircController = std::make_shared<SpircController>(mercuryManager, blob->username, audioSink);

  //             spircController->setEventHandler([](CSpotEvent& event) {
  //                 switch (event.eventType) {
  //                 case CSpotEventType::TRACK_INFO:
  //                     CSPOT_LOG(info, "Track Info");
  //                     break;
  //                 case CSpotEventType::PLAY_PAUSE:
  //                     CSPOT_LOG(info, "Track Pause");
  //                     break;
  //                 case CSpotEventType::SEEK:
  //                     CSPOT_LOG(info, "Track Seek");
  //                     break;
  //                 case CSpotEventType::DISC:
  //                     CSPOT_LOG(info, "Disconnect");
  //                     spircController->stopPlayer();
  //                     mercuryManager->stop();
  //                     break;
  //                 case CSpotEventType::PREV:
  //                     CSPOT_LOG(info, "Track Previous");
  //                     break;
  //                 case CSpotEventType::NEXT:
  //                     CSPOT_LOG(info, "Track Next");
  //                     break;
  //                 default:
  //                     break;
  //                 }
  //                 });

  //             mercuryManager->reconnectedCallback = []() {
  //                 return spircController->subscribe();
  //             };

  //             mercuryManager->handleQueue();

  //             mercuryManager.reset();
  //             spircController.reset();
  //         }

  //         BELL_SLEEP_MS(10000);
  //         //        heap_trace_stop();
  //         //        heap_trace_dump();
  //         ESP_LOGI(TAG, "Player exited");
  //         auto memUsage = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  //         auto memUsage2 = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

  //         BELL_LOG(info, "esp32", "Free RAM %d | %d", memUsage, memUsage2);
  //     };

  //     createdFromZeroconf = true;
  //     auto httpServer = std::make_shared<bell::HTTPServer>(2137);
  //     auto authenticator = std::make_shared<ZeroconfAuthenticator>(createPlayerCallback, httpServer);
  //     authenticator->registerHandlers();
  //     httpServer->listen();

  vTaskSuspend(NULL);
}

void init_spiffs() {
  esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
             esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
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

  ESP_LOGI(TAG, "Connected to AP, start spotify receiver");
  //auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 12*1024, NULL, 5, NULL, 1);
  /*auto taskHandle = */
  bell::setDefaultLogger();

  auto task = std::make_unique<CSpotTask>();
  vTaskSuspend(NULL);
}
