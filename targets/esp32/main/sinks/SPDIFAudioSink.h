#ifndef SPDIFAUDIOSINK_H
#define SPDIFAUDIOSINK_H

#include <vector>
#include <iostream>
#include "BufferedAudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"

class SPDIFAudioSink : public BufferedAudioSink
{
public:
    SPDIFAudioSink();
    ~SPDIFAudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
private:
};

#endif