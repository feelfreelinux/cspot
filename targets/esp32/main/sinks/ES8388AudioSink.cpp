#include "ES8388AudioSink.h"

struct es8388_cmd_s {
    uint8_t reg;
    uint8_t value;
};

ES8388AudioSink::ES8388AudioSink()
{
    // configure i2c
    i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 33,
        .scl_io_num = 32,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
    };

    i2c_config.master.clk_speed = 100000;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Only TX
        .sample_rate = 44100,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = 0, //Default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = true,
        .tx_desc_auto_clear = true, //Auto clear tx descriptor on underflow
        .fixed_mclk = 256 * 44100
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 27,
        .ws_io_num = 25,
        .data_out_num = 26,
        .data_in_num = -1 //Not used
    };

    int err;

    err = i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE("OI", "i2s driver installation error: %d", err);
    }

    err = i2s_set_pin((i2s_port_t)0, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE("OI", "i2s set pin error: %d", err);
    }

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);
    ESP_LOGI("OI", "MCLK output on CLK_OUT1");


    err = i2c_param_config(0, &i2c_config);
    if (err != ESP_OK) {
        ESP_LOGE("OI", "i2c param config error: %d", err);
    }
    
    err = i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE("OI", "i2c driver installation error: %d", err);
    }

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();

    err = i2c_master_start(i2c_cmd);
    if (err != ESP_OK) {
        ESP_LOGE("OI", "i2c master start error: %d", err);
    }

    /* mute DAC during setup, power up all systems, slave mode */
    writeReg(ES8388_DACCONTROL3, 0x04); //0x19 -> 25, 
    writeReg(ES8388_CONTROL2, 0x50); // 0x01 -> 2, , Chip Power Management, resets DAC
    writeReg(ES8388_MASTERMODE, 0x00);

    /* power up DAC and enable LOUT1+2 / ROUT1+2, ADC sample rate = DAC sample rate */
    writeReg(ES8388_DACPOWER, 0x3c);
    writeReg(ES8388_CONTROL1, 0x12); // 0x00 -> 0, Chip Control 1,

    /* DAC I2S setup: 16 bit word length, I2S format; MCLK / Fs = 256*/
    writeReg(ES8388_DACCONTROL1, 0x18); // 0x17 -> 23, 16 bit word length
    writeReg(ES8388_DACCONTROL2, 0x02); // Fs = 256

    /* DAC and ADC use same LRCK, enable MCLK input; output resistance setup */
    writeReg(ES8388_DACCONTROL21, 0x80); // 0x2b -> 43, DAC Control 21,  DACLRC and ADCLRC same

    /* DAC volume control: 0dB (maximum, unattented)  */
    writeReg(ES8388_DACCONTROL4, 0x00); // 0x1a -> 26, DAC Control 4, left Digital volume control attenuates 0dB
    writeReg(ES8388_DACCONTROL5, 0x00); // 0x1a -> 27, DAC Control 5, right Digital volume control attenuates 0dB

    /* power down ADC while configuring */
    writeReg(ES8388_ADCPOWER, 0xff); // 0x03 -> 3, ADC Power Management

    /* set LOUT1 / ROUT1 volume: 0dB (unattenuated) */
    writeReg(ES8388_DACCONTROL24, 0x1e); // 0x2e -> 46, LOUT1 volume - 0dB
    writeReg(ES8388_DACCONTROL25, 0x1e); // 0x2f -> 47, ROUT1 volume - 0dB

    /* set LOUT2 / ROUT2 volume: 0dB (unattenuated) */
    writeReg(ES8388_DACCONTROL26, 0x1e); // 0x30 -> 46, LOUT2 volume - 0dB
    writeReg(ES8388_DACCONTROL27, 0x1e); // 0x31 -> 46, ROUT2 volume - 0dB

    /* power up and enable DAC; power up ADC (no MIC bias) */
    writeReg(ES8388_DACPOWER, 0x3c);  // 0x04 -> 4, DAC Power Management, 
    writeReg(ES8388_DACCONTROL3, 0x00); // 0x19 -> 25,  DAC Control 3 - 0.5 dB per 4 LRCK digital volume control ramp rate

    // Enable GPIO 21 to enable the amplifier
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT64(GPIO_NUM_21);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_21, 1);

    startI2sFeed();
}

void ES8388AudioSink::writeReg(uint8_t reg_add, uint8_t data)
{

    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES8388_ADDR, ACK_CHECK_EN);
    res |= i2c_master_write_byte(cmd, reg_add, ACK_CHECK_EN);
    res |= i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (res != ESP_OK) {
        ESP_LOGE("RR", "Unable to write to ES8388: %d", res);
    }else{
        ESP_LOGE("RR", "register successfull written.");
    }
}

ES8388AudioSink::~ES8388AudioSink()
{
}
