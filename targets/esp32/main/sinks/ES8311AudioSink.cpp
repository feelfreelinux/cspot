#include "ES8311AudioSink.h"
#include "driver/i2s.h"

ES8311AudioSink::ES8311AudioSink()
{
    softwareVolumeControl = false;
    // usign = true;

    board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    i2s_config_t i2s_config = {

        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,
        .dma_buf_count = 3,
        .dma_buf_len = 300,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_pin_config_t i2s_pin_cfg = { 0 };
    get_i2s_pins(I2S_NUM_0, &i2s_pin_cfg);
    i2s_set_pin(I2S_NUM_0, &i2s_pin_cfg);

    i2s_mclk_gpio_select(I2S_NUM_0, GPIO_NUM_0);

    startI2sFeed();
}

void ES8311AudioSink::volumeChanged(uint16_t volume) {
    volume = (volume+1) >> 10; //this isn't exactly right
    ESP_LOGW("VOL", "SET: volume:%d", volume);
    board_handle->audio_hal->audio_codec_set_volume(volume);
}

ES8311AudioSink::~ES8311AudioSink()
{
}
