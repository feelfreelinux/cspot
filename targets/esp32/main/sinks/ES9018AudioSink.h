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

class ES9018AudioSink : public AudioSink
{
public:
    ES9018AudioSink();
    ~ES9018AudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
private:
};

#endif