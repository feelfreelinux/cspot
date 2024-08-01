
#include <dirent.h>
#include <driver/gpio.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

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
  if (CONFIG_GPIO_SD_CS >= 0) {
    sdspi_device_config_t sd_device = SDSPI_DEVICE_CONFIG_DEFAULT();
    sd_device.gpio_cs = (gpio_num_t)CONFIG_GPIO_SD_CS;
    sd_device.gpio_int = (gpio_num_t)-1;
    gpio_install_isr_service(0);
    sdspi_dev_handle_t sd_spi_handle;
    ret = sdspi_host_init();
    ESP_ERROR_CHECK(ret);
    ret = sdspi_host_init_device(&sd_device, &sd_spi_handle);
    ESP_ERROR_CHECK(ret);
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = sd_spi_handle;
    gpio_reset_pin((gpio_num_t)CONFIG_GPIO_SD_CS);
    gpio_set_direction((gpio_num_t)CONFIG_GPIO_SD_CS, GPIO_MODE_OUTPUT);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif  // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    ESP_LOGI(TAG, "Initializing SD card");
    /*
                ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &sd_device, &mount_config, &card);
                if (ret != ESP_OK) {
                    if (ret == ESP_FAIL) {
                        ESP_LOGE(TAG, "Failed to mount filesystem. "
                            "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
                    }
                    else {
                        ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                            "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
                    }
                }
                */
    sdmmc_card_t* card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));

    ESP_LOGI(TAG, "Filesystem mounted");
    if (ret == ESP_OK)
      sdmmc_card_print_info(stdout, card);
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &sd_device, &mount_config,
                                  &card);
    if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
        ESP_LOGE(TAG,
                 "Failed to mount filesystem. "
                 "If you want the card to be formatted, set the "
                 "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
      } else {
        ESP_LOGE(TAG,
                 "Failed to initialize the card (%s). "
                 "Make sure SD card lines have pull-up resistors in place.",
                 esp_err_to_name(ret));
      }
    }
    struct dirent* entry;
    struct stat file_stat;
    DIR* dir = opendir(MOUNT_POINT);
    char n_name[280];
    if (!dir)
      printf("no dir\n");
    while ((entry = readdir(dir)) != NULL) {
      sprintf(n_name, "%s/%s", MOUNT_POINT, entry->d_name);
      printf("%s\n", n_name);
      printf("name:%s, ino: %i\n", entry->d_name, entry->d_ino);
      printf("stat_return_value:%i\n", stat(n_name, &file_stat));
      printf("stat_mode:%i\n", file_stat.st_mode);
    }
    dir = opendir(MOUNT_POINT "/TRACKS");
    if (!dir)
      printf("no dir\n");
    while ((entry = readdir(dir)) != NULL) {
      printf("%s\n", entry->d_name);
    }
    closedir(dir);
  }
  VS1053->write_register(SCI_VOL, 10 | 10 << 8);
}