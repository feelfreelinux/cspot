#ifndef PCM5102AUDIOSINK_H
#define PCM5102AUDIOSINK_H

#include <vector>
#include <iostream>
#include "BufferedAudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"

class PCM5102AudioSink : public BufferedAudioSink
{
public:
    PCM5102AudioSink();
    ~PCM5102AudioSink();
private:
};

#endif