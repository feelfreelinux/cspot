#ifndef BUFFEREDAUDIOSINK_H
#define BUFFEREDAUDIOSINK_H

#include <vector>
#include <iostream>
#include "AudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"

class BufferedAudioSink : public AudioSink
{
public:
    void feedPCMFrames(std::vector<uint8_t> &data);
protected:
    void startI2sFeed(size_t buf_size = 4096 * 8);
    void feedPCMFramesInternal(const void *pvItem, size_t xItemSize);
private:
};

#endif