
#define SPOTI_LOGIN "login"
#define SPOTI_PASSWORD "password"

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_sntp.h"
#include <time.h>
#include "esp_wifi.h"
#include <sys/time.h>
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <string.h>
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
#include <ApResolve.h>
#include <inttypes.h>
#include <I2SAudioSink.h>
#include "freertos/task.h"
#include "freertos/ringbuf.h"

static const char *TAG = "cspot";

extern "C"
{
    void app_main(void);
}
static void cspotTask(void *pvParameters)
{
    auto apResolver = std::make_shared<ApResolve>();
    auto connection = std::make_shared<PlainConnection>();

    auto apAddr = apResolver->fetchFirstApAddress();
    connection->connectToAp(apAddr);

    auto session = std::make_unique<Session>();
    session->connect(connection);
    auto token = session->authenticate(SPOTI_LOGIN, SPOTI_PASSWORD);

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(session->shanConn);
        mercuryManager->startTask();
        auto audioSink = std::make_shared<I2SAudioSink>();
        auto spircController = std::make_shared<SpircController>(mercuryManager, SPOTI_LOGIN, audioSink);
        mercuryManager->handleQueue();
    }
}
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
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
    // esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    ESP_LOGI("TAG", "Connected to AP, start spotify receiver");

    auto taskHandle = xTaskCreatePinnedToCore(&cspotTask, "cspot", 8192 * 8, NULL, 5, NULL, 0);
}
