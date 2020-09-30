#include "ES9018AudioSink.h"
#define typeof(x) __typeof__(x)
#include "driver/i2s.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "driver/i2c.h"
static gpio_num_t i2c_gpio_sda = (gpio_num_t) 21;
static gpio_num_t i2c_gpio_scl = (gpio_num_t) 22;
static uint32_t i2c_frequency = 100000;
static i2c_port_t i2c_port = I2C_NUM_0;

RingbufHandle_t dataBuffer;

static void i2sFeed(void *pvParameters)
{
    while (true)
    {
        size_t itemSize;
        char *item = (char *)xRingbufferReceiveUpTo(dataBuffer, &itemSize, portMAX_DELAY, 512);
        if (item != NULL)
        {
            //vTaskDelay(1);
            size_t written = 0;
            while (written < itemSize)
            {
                i2s_write((i2s_port_t)0, item, itemSize, &written, portMAX_DELAY);
            }
            vRingbufferReturnItem(dataBuffer, (void *)item);
        }
    }
}

ES9018AudioSink::ES9018AudioSink()
{
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_gpio_sda,
        .scl_io_num = i2c_gpio_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE};
    conf.master.clk_speed = i2c_frequency;
    i2c_param_config(i2c_port, &conf);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x48 << 1 | I2C_MASTER_WRITE, 0x1);
    i2c_master_write_byte(cmd, 1, 0x1);
    i2c_master_write_byte(cmd, 0x0C, 0x1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK)
    {
        ESP_LOGI("cspot", "Write OK");
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGW("cspot", "Bus is busy");
    }
    else
    {
        ESP_LOGW("cspot", "Write Failed");
    }
    i2c_driver_delete(i2c_port);

    dataBuffer = xRingbufferCreate(4096 * 8, RINGBUF_TYPE_BYTEBUF);

    i2s_config_t i2s_config = {

        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Only TX
        .sample_rate = 44100,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0, //Default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = true,
        .tx_desc_auto_clear = true //Auto clear tx descriptor on underflow
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 27,
        .ws_io_num = 32,
        .data_out_num = 25,
        .data_in_num = -1 //Not used
    };
    i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
    i2s_set_pin((i2s_port_t)0, &pin_config);

    xTaskCreatePinnedToCore(&i2sFeed, "i2sFeed", 4096, NULL, 10, NULL, 1);
}

ES9018AudioSink::~ES9018AudioSink()
{
    // this->namedPipeFile.close();
}

// size_t written = 0;
// while (written < data.size()) {
//     i2s_write((i2s_port_t)0, (const char *)&data[0], data.size(), &written, portMAX_DELAY);
// }
void ES9018AudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    // Write the actual data
    // size_t written = 0;
    //             i2s_write((i2s_port_t)0, data.data(), data.size(), &written, portMAX_DELAY);
    xRingbufferSend(dataBuffer, &data[0], data.size(), portMAX_DELAY);
}
