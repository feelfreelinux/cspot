#include "InternalAudioSink.h"
#include "driver/i2s.h"

InternalAudioSink::InternalAudioSink()
{
    softwareVolumeControl = true;

    i2s_config_t i2s_config = {

        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100,
        .bits_per_sample = (i2s_bits_per_sample_t)16, // Internal DAC will use only top 8 bits
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0, //Default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = true,
        .tx_desc_auto_clear = true //Auto clear tx descriptor on underflow
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);

    startI2sFeed();
}

InternalAudioSink::~InternalAudioSink()
{
}

//void AC101AudioSink::volumeChanged(uint16_t volume) {
//    dac->volume(volume, volume);
//}
