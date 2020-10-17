#ifndef PORTAUDIOSINK_H
#define PORTAUDIOSINK_H
#ifdef CSPOT_ENABLE_PORTAUDIO_SINK

#include <vector>
#include "portaudio.h"
#include <stdint.h>
#include <iostream>
#include "AudioSink.h"

class PortAudioSink : public AudioSink
{
public:
    PortAudioSink();
    ~PortAudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
    
private:
    PaStream *stream;
};

#endif
#endif