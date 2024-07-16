
#include <dirent.h>
#include <driver/sdspi_host.h>
#include "esp_log.h"

#include "VS1053.h"

#define MOUNT_POINT "/sdcard"

SemaphoreHandle_t SPI_semaphore;
#define TAG "INIT"
void initAudioSink(std::shared_ptr<VS1053_SINK> VS1053) {
  esp_err_t ret;
  // SPI SETUP
  spi_bus_config_t bus_cfg;
  memset(&bus_cfg, 0, sizeof(spi_bus_config_t));
  bus_cfg.sclk_io_num = CONFIG_GPIO_CLK;
  bus_cfg.mosi_io_num = CONFIG_GPIO_MOSI;
  bus_cfg.miso_io_num = CONFIG_GPIO_MISO;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  ESP_LOGI("vsInit", "spi config done");
  ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, 1);
  assert(ret == ESP_OK);
  SPI_semaphore = xSemaphoreCreateMutex();

  // VS1053 SETUP
  VS1053->init(HSPI_HOST, &SPI_semaphore);
  VS1053->write_register(SCI_VOL, 10 | 10 << 8);
}