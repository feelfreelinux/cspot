#ifndef I2SAUDIOSINK_H
#define I2SAUDIOSINK_H

#include <vector>
#include <iostream>
#include "AudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

class I2SAudioSink : public AudioSink
{
public:
    I2SAudioSink();
    ~I2SAudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
    FILE* file;
private:
};

#endif