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
#include "ProtoHelper.h"
#include "Logger.h"

// Config
#define SINK        INTERNAL // INTERNAL, AC101, ES8018, PCM5102
#define QUALITY     320      // 320, 160, 96
#define DEVICE_NAME "CSpot"

#if SINK == INTERNAL
#include <InternalAudioSink.h>
#elif SINK == AC101
#include <AC101AudioSink.h>
#elif SINK == ES8018
#include <ES9018AudioSink.h>
#elif SINK == PCM5102
#include <PCM5102AudioSink.h>
#endif

static const char *TAG = "cspot";

std::shared_ptr<ConfigJSON> configMan;

extern "C"
{
    void app_main(void);
}
static void cspotTask(void *pvParameters)
{
    auto zeroconfAuthenticator = std::make_shared<ZeroconfAuthenticator>();

    // Config file
    auto file = std::make_shared<ESPFile>();
    configMan = std::make_shared<ConfigJSON>("/spiffs/config.json", file);

    if(!configMan->load())
    {
      CSPOT_LOG(error, "Config error");
    }

    configMan->deviceName = DEVICE_NAME;

#if QUALITY == 320
    configMan->format = AudioFormat::OGG_VORBIS_320;
#elif QUALITY == 160
    configMan->format = AudioFormat::OGG_VORBIS_160;
#else
    configMan->format = AudioFormat::OGG_VORBIS_96;
#endif

    // Blob file
    std::string credentialsFileName = "/spiffs/authBlob.json";
    std::shared_ptr<LoginBlob> blob;
    std::string jsonData;

    bool read_status = file->readFile(credentialsFileName, jsonData);

    if(jsonData.length() > 0 && read_status)
    {
      blob = std::make_shared<LoginBlob>();
      blob->loadJson(jsonData);
    }
    else
    {
      auto authenticator = std::make_shared<ZeroconfAuthenticator>();
      blob = authenticator->listenForRequests();
      file->writeFile(credentialsFileName, blob->toJson());
    }

    auto session = std::make_unique<Session>();
    session->connectWithRandomAp();
    auto token = session->authenticate(blob);

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(std::move(session));
        mercuryManager->startTask();

#if SINK == INTERNAL
        auto audioSink = std::make_shared<InternalAudioSink>();
#elif SINK == AC101
        auto audioSink = std::make_shared<AC101AudioSink>();
#elif SINK == ES8018
        auto audioSink = std::make_shared<ES9018AudioSink>();
#elif SINK == PCM5102
        auto audioSink = std::make_shared<PCM5102AudioSink>();
#endif

        auto spircController = std::make_shared<SpircController>(mercuryManager, blob->username, audioSink);
        mercuryManager->reconnectedCallback = [spircController]() {
            return spircController->subscribe();
        };
        mercuryManager->handleQueue();
    }
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

    ESP_LOGI("TAG", "Connected to AP, start spotify receiver");
    // for(;;);
    auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 8192 * 8, NULL, 5, NULL, 0);
}
