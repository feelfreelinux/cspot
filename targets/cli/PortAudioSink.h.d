#ifndef PORTAUDIOSINK_H
#define PORTAUDIOSINK_H
#include "portaudio.h"

#include <vector>
#include <fstream>
#include "AudioSink.h"

class PortAudioSink : public AudioSink
{
public:
    PortAudioSink();
    ~PortAudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
    
private:
    std::ofstream namedPipeFile;
};

#endif