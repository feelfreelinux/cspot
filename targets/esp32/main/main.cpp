#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include <string.h>
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "mdns.h"
#include <string>
#include <PlainConnection.h>
#include <Session.h>
#include <SpircController.h>
#include <MercuryManager.h>
#include <ZeroconfAuthenticator.h>

#include <ApResolve.h>
#include <inttypes.h>
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "ConfigJSON.h"
#include "ESPFile.h"
#include "Logger.h"
#include <HTTPServer.h>
#include "BellUtils.h"



// Config sink
#define AC101 // INTERNAL, AC101, ES8018, PCM5102
#define QUALITY     320      // 320, 160, 96
#define DEVICE_NAME "CSpot-ESP32"

#ifdef INTERNAL
#include <InternalAudioSink.h>
#endif
#ifdef AC101
#include <AC101AudioSink.h>
#endif
#ifdef ES8018
#include <ES9018AudioSink.h>
#endif
#ifdef PCM5102
#include <PCM5102AudioSink.h>
#endif

static const char *TAG = "cspot";

std::shared_ptr<ESPFile> file;
std::shared_ptr<MercuryManager> mercuryManager;
std::shared_ptr<SpircController> spircController;
std::shared_ptr<ConfigJSON> configMan;
std::string credentialsFileName = "/spiffs/authBlob.json";
bool createdFromZeroconf = false;

extern "C"
{
void app_main(void);
}

static void cspotTask(void *pvParameters)
{

    bell::setDefaultLogger();

    mdns_init();
    mdns_hostname_set("cspot");

#ifdef INTERNAL
    auto audioSink = std::make_shared<InternalAudioSink>();
#endif
#ifdef AC101
    auto audioSink = std::make_shared<AC101AudioSink>();
#endif
#ifdef ES8018
    auto audioSink = std::make_shared<ES8018AudioSink>();
#endif
#ifdef PCM5102
    auto audioSink = std::make_shared<PCM5102AudioSink>();
#endif

    // Config file
    file = std::make_shared<ESPFile>();
    configMan = std::make_shared<ConfigJSON>("/spiffs/config.json", file);

    if(!configMan->load()) {
        CSPOT_LOG(error, "Config error");
    }

    configMan->deviceName = DEVICE_NAME;
    configMan->format = AudioFormat_OGG_VORBIS_160;

    auto createPlayerCallback = [audioSink](std::shared_ptr<LoginBlob> blob) {



//        heap_trace_start(HEAP_TRACE_LEAKS);
//        esp_dump_per_task_heap_info();


        CSPOT_LOG(info, "Creating player");

        auto session = std::make_unique<Session>();
        session->connectWithRandomAp();
        auto token = session->authenticate(blob);

        // Auth successful
        if (token.size() > 0)
        {
            if (createdFromZeroconf) {
                file->writeFile(credentialsFileName, blob->toJson());
            }


            mercuryManager = std::make_shared<MercuryManager>(std::move(session));
            mercuryManager->startTask();

            spircController = std::make_shared<SpircController>(mercuryManager, blob->username, audioSink);

            spircController->setEventHandler([](CSpotEvent &event) {
                switch (event.eventType) {
                    case CSpotEventType::TRACK_INFO:
                        CSPOT_LOG(info, "Track Info");
                        break;
                    case CSpotEventType::PLAY_PAUSE:
                        CSPOT_LOG(info, "Track Pause");
                        break;
                    case CSpotEventType::SEEK:
                        CSPOT_LOG(info, "Track Seek");
                        break;
                    case CSpotEventType::DISC:
                        CSPOT_LOG(info, "Disconnect");
                        spircController->stopPlayer();
                        mercuryManager->stop();
                        break;
                    case CSpotEventType::PREV:
                        CSPOT_LOG(info, "Track Previous");
                        break;
                    case CSpotEventType::NEXT:
                        CSPOT_LOG(info, "Track Next");
                        break;
                    default:
                        break;
                }
            });

            mercuryManager->reconnectedCallback = []() {
                return spircController->subscribe();
            };

            mercuryManager->handleQueue();

            mercuryManager.reset();
            spircController.reset();
        }

        BELL_SLEEP_MS(10000);
//        heap_trace_stop();
//        heap_trace_dump();
        ESP_LOGI(TAG, "Player exited");
        auto memUsage = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        auto memUsage2 = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

        BELL_LOG(info, "esp32", "Free RAM %d | %d", memUsage, memUsage2);
    };

    createdFromZeroconf = true;
    auto httpServer = std::make_shared<bell::HTTPServer>(2137);
    auto authenticator = std::make_shared<ZeroconfAuthenticator>(createPlayerCallback, httpServer);
    authenticator->registerHandlers();
    httpServer->listen();

    vTaskSuspend(NULL);

}

void init_spiffs()
{
    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_spiffs();

    esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "Connected to AP, start spotify receiver");
    //auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 12*1024, NULL, 5, NULL, 1);
    auto taskHandle = xTaskCreate(&cspotTask, "cspot", 12*1024, NULL, 5, NULL);
}
