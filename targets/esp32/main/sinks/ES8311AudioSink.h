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

#include "board.h"

class ES8311AudioSink : public BufferedAudioSink
{
public:
    ES8311AudioSink();
    ~ES8311AudioSink();
    void volumeChanged(uint16_t volume);
private:
    audio_board_handle_t board_handle;
};

#endif
