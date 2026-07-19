// Pantalla 2.8" IPS ILI9341V + touch FT6336G de la placa ES3C28P/ES3N28P.
// Pines tomados del demo del fabricante (2.8inch_ESP32-S3_LVGL):
//   SPI2: MOSI=11 CLK=12 MISO=13 CS=10 DC=46 RST=18, backlight=45 (activo alto)
//   I2C0: SDA=16 SCL=15 (compartido con codec ES8311 y sensor SHT31)
//   Landscape 320x240: MADCTL 0x28 -> swap_xy, sin mirror, orden BGR,
//   colores invertidos (panel IPS).

#include "board_display.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#define LCD_HOST SPI2_HOST
#define PIN_LCD_MOSI 11
#define PIN_LCD_CLK 12
#define PIN_LCD_MISO 13
#define PIN_LCD_CS 10
#define PIN_LCD_DC 46
#define PIN_LCD_RST 18
#define PIN_LCD_BCKL 45

#define I2C_PORT I2C_NUM_0
#define PIN_I2C_SDA 16
#define PIN_I2C_SCL 15

#define LCD_H_RES 320
#define LCD_V_RES 240

static const char* TAG = "board_display";

// Instala el driver I2C legado en el puerto 0. El ES8311 (bell) reutiliza
// este bus: su I2cInit tolera el driver ya instalado.
static void board_i2c_init(void) {
  i2c_config_t cfg = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = PIN_I2C_SDA,
      .scl_io_num = PIN_I2C_SCL,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master = {.clk_speed = 100000},
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0));
}

void board_display_init(void) {
  board_i2c_init();

  // Backlight apagado durante el init para no mostrar basura
  gpio_config_t bk_cfg = {
      .pin_bit_mask = 1ULL << PIN_LCD_BCKL,
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&bk_cfg));
  gpio_set_level(PIN_LCD_BCKL, 0);

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = PIN_LCD_MOSI,
      .miso_io_num = PIN_LCD_MISO,
      .sclk_io_num = PIN_LCD_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_cfg = {
      .cs_gpio_num = PIN_LCD_CS,
      .dc_gpio_num = PIN_LCD_DC,
      .spi_mode = 0,
      .pclk_hz = 40 * 1000 * 1000,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                           &io_cfg, &io_handle));

  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_cfg = {
      .reset_gpio_num = PIN_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
      .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // Touch FT6336G (compatible FT5x06) por I2C legado, sin INT: polling
  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t tp_io_cfg =
      ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_PORT,
                                           &tp_io_cfg, &tp_io_handle));

  esp_lcd_touch_handle_t tp = NULL;
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = LCD_H_RES,
      .y_max = LCD_V_RES,
      .rst_gpio_num = -1,
      .int_gpio_num = -1,
      .flags =
          {
              // Equivalente a SWAPXY + INVERT_Y del demo
              .swap_xy = 1,
              .mirror_x = 0,
              .mirror_y = 1,
          },
  };
  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

  const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = LCD_H_RES * 24,
      .double_buffer = true,
      .hres = LCD_H_RES,
      .vres = LCD_V_RES,
      .monochrome = false,
      .rotation =
          {
              .swap_xy = true,
              .mirror_x = false,
              .mirror_y = false,
          },
      .color_format = LV_COLOR_FORMAT_RGB565,
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = true,
          },
  };
  lv_display_t* disp = lvgl_port_add_disp(&disp_cfg);

  const lvgl_port_touch_cfg_t touch_cfg = {
      .disp = disp,
      .handle = tp,
  };
  lvgl_port_add_touch(&touch_cfg);

  gpio_set_level(PIN_LCD_BCKL, 1);
  ESP_LOGI(TAG, "Pantalla + touch listos");
}
