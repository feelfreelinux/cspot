#ifndef TAS5711AUDIOSINK_H
#define TAS5711AUDIOSINK_H


#include "driver/i2s.h"
#include <driver/i2c.h>
#include <vector>
#include <iostream>
#include "BufferedAudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"

class TAS5711AudioSink : public BufferedAudioSink
{
public:
    TAS5711AudioSink();
    ~TAS5711AudioSink();


    void writeReg(uint8_t reg, uint8_t value);
private:
    i2c_config_t i2c_config;
    i2c_port_t i2c_port = 0;
};

#endif