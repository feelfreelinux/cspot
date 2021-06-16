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

class InternalAudioSink : public BufferedAudioSink
{
public:
    InternalAudioSink();
    ~InternalAudioSink();
private:
};

#endif
