#ifndef ESPSTATUSLED_H
#define ESPSTATUSLED_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define CSPOT_STATUS_LED_GPIO (gpio_num_t)CONFIG_CSPOT_STATUS_LED_GPIO

enum StatusLed {
    IDLE,
    ERROR,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    SPOT_INITIALIZING,
    SPOT_READY
};

class ESPStatusLed
{
public:
    ESPStatusLed();

    void setStatus(StatusLed);
private:
    StatusLed status = IDLE;
#ifdef CONFIG_CSPOT_STATUS_LED_TYPE_RMT
    led_strip_t* pStrip_a;
#endif
};

#endif
