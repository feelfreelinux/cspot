// Sensor SHT31-D en el bus I2C0 compartido (SDA=16, SCL=15).
// Lectura single-shot de alta repetibilidad cada 5 s.

#include "sht31.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui.h"

#define SHT31_ADDR 0x44
#define I2C_PORT I2C_NUM_0

static const char* TAG = "sht31";

// CRC-8 de Sensirion: polinomio 0x31, init 0xFF
static uint8_t sht31_crc(const uint8_t* data, int len) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
    }
  }
  return crc;
}

static esp_err_t sht31_read(float* temp_c, float* hum_pct) {
  const uint8_t cmd[2] = {0x24, 0x00};  // single-shot, high repeatability
  esp_err_t err = i2c_master_write_to_device(I2C_PORT, SHT31_ADDR, cmd, 2,
                                             pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    return err;
  }
  vTaskDelay(pdMS_TO_TICKS(20));  // conversion: max 15 ms

  uint8_t buf[6];
  err = i2c_master_read_from_device(I2C_PORT, SHT31_ADDR, buf, sizeof(buf),
                                    pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    return err;
  }
  if (sht31_crc(buf, 2) != buf[2] || sht31_crc(buf + 3, 2) != buf[5]) {
    return ESP_ERR_INVALID_CRC;
  }

  uint16_t raw_t = (buf[0] << 8) | buf[1];
  uint16_t raw_h = (buf[3] << 8) | buf[4];
  *temp_c = -45.0f + 175.0f * (float)raw_t / 65535.0f;
  *hum_pct = 100.0f * (float)raw_h / 65535.0f;
  return ESP_OK;
}

static void sht31_task(void* arg) {
  int failures = 0;
  while (1) {
    float t, h;
    esp_err_t err = sht31_read(&t, &h);
    if (err == ESP_OK) {
      failures = 0;
      ui_set_env(t, h);
    } else if (++failures == 3) {
      ESP_LOGW(TAG, "Lectura SHT31 falla: %s", esp_err_to_name(err));
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void sht31_start(void) {
  xTaskCreate(sht31_task, "sht31", 3072, NULL, 3, NULL);
}
