#ifndef AUDIOSINK_H
#define AUDIOSINK_H

#include <vector>

class AudioSink
{
public:
    AudioSink() {}
    virtual ~AudioSink() {}
    virtual void feedPCMFrames(std::vector<uint8_t> &data) = 0;
    virtual void volumeChanged(uint16_t volume) {}
    bool softwareVolumeControl = true;
    bool usign = false;
};

#endif
