#ifndef NAMEDPIPEAUDIOSINK_H
#define NAMEDPIPEAUDIOSINK_H

#include <vector>
#include <fstream>
#include "AudioSink.h"

class NamedPipeAudioSink : public AudioSink
{
public:
    NamedPipeAudioSink();
    ~NamedPipeAudioSink();
    void feedPCMFrames(std::vector<uint8_t> &data);
    
private:
    std::ofstream namedPipeFile;
};

#endif