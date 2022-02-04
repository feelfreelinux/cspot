#include "ESPStatusLed.h"

static const char* TAG = "statusled";

#define CSPOT_STATUS_LED_STATUSES 6
#define CSPOT_STATUS_LED_STATES   2

#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_GPIO
static uint32_t StatusLedBlinkTimings[CSPOT_STATUS_LED_STATUSES][CSPOT_STATUS_LED_STATES] = {
    // on, off - in milliseconds
    {1024, 16}, // IDLE
    {512, 512}, // ERROR
    {256, 512}, // WIFI_CONNECTING
    {256, 1024}, // WIFI_CONNECTED
    {64, 512}, // SPOT_INITIALIZING
    {64, 1024}  // SPOT_READY
};
#endif

#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_RMT
static uint32_t StatusLedBlinkTimings[CSPOT_STATUS_LED_STATUSES][CSPOT_STATUS_LED_STATES + 1] = {
    // on, off - in milliseconds, color (R,G,B)
    {1024, 16, 0x000080}, // IDLE
    {512, 512, 0x800000}, // ERROR
    {128, 512, 0x808000}, // WIFI_CONNECTING
    {128, 1024, 0x808000}, // WIFI_CONNECTED
    {64, 512, 0x008000}, // SPOT_INITIALIZING
    {64, 1024, 0x008000}  // SPOT_READY
};
#endif

ESPStatusLed::ESPStatusLed() {

#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_NONE
    ESP_LOGI(TAG, "Status LED is disabled");
#endif

#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_GPIO
    ESP_LOGI(TAG, "Enabling status LED in GPIO mode");
    gpio_reset_pin(CSPOT_STATUS_LED_GPIO);
    gpio_set_direction(CSPOT_STATUS_LED_GPIO, GPIO_MODE_OUTPUT);
#endif

#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_RMT
    ESP_LOGI(TAG, "Enabling status LED in RGB mode");
    pStrip_a = led_strip_init(CONFIG_CSPOT_STATUS_LED_RMT_CHANNEL, CSPOT_STATUS_LED_GPIO, 1);
    pStrip_a->clear(pStrip_a, 50);
#endif

#if defined(CONFIG_CSPOT_STATUS_LED_TYPE_GPIO) || defined(CONFIG_CSPOT_STATUS_LED_TYPE_RMT)
    xTaskCreate([](void* _self) {
        ESPStatusLed* self = (ESPStatusLed*)_self;
        
        while (1) {
            //ESP_LOGI(TAG, "Led is ON, status = %d, wait = %d", self->status, StatusLedBlinkTimings[(int)self->status][0]);
            #ifdef CONFIG_CSPOT_STATUS_LED_TYPE_GPIO
            gpio_set_level(CSPOT_STATUS_LED_GPIO, 1);
            #endif
            #ifdef CONFIG_CSPOT_STATUS_LED_TYPE_RMT
            uint32_t color = StatusLedBlinkTimings[(int)self->status][2];
            self->pStrip_a->set_pixel(self->pStrip_a, 0, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
            self->pStrip_a->refresh(self->pStrip_a, 100);
            #endif
            vTaskDelay(StatusLedBlinkTimings[(int)self->status][0] / portTICK_PERIOD_MS);

            //ESP_LOGI(TAG, "Led is OFF, status = %d, wait = %d", self->status, StatusLedBlinkTimings[(int)self->status][1]);
            #ifdef CONFIG_CSPOT_STATUS_LED_TYPE_GPIO
            gpio_set_level(CSPOT_STATUS_LED_GPIO, 0);
            #endif
            #ifdef CONFIG_CSPOT_STATUS_LED_TYPE_RMT
            self->pStrip_a->clear(self->pStrip_a, 50);
            #endif
            vTaskDelay(StatusLedBlinkTimings[(int)self->status][1] / portTICK_PERIOD_MS);
        }
    }, "statusled", 2 * 1024, (void*)this, 6, NULL);
#endif
}

void ESPStatusLed::setStatus(StatusLed newStatus)
{
    ESP_LOGI(TAG, "Chaing status to %d", this->status);
    this->status = newStatus;
}