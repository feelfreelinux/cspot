#ifndef INTERNALAUDIOSINK_H
#define INTERNALAUDIOSINK_H

#include <vector>
#include <iostream>
#include "BufferedAudioSink.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
//#include "ac101.h"
//#include "adac.h"

class InternalAudioSink : public BufferedAudioSink
{
public:
    InternalAudioSink();
    ~InternalAudioSink();
//    void volumeChanged(uint16_t volume);
private:
//    adac_s *dac;
};

#endif
