#ifndef AC101AUDIOSINK_H
#define AC101AUDIOSINK_H

#include <vector>
#include <iostream>
#include "AudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "ac101.h"
#include "adac.h"

class AC101AudioSink : public AudioSink
{
public:
    AC101AudioSink();
    ~AC101AudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
    void volumeChanged(uint16_t volume);
private:
    adac_s *dac;
};

#endif