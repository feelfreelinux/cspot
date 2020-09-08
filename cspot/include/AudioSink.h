#ifndef AUDIOSINK_H
#define AUDIOSINK_H

#include <vector>

class AudioSink
{
public:
    AudioSink() {}
    virtual ~AudioSink() {}
    virtual void feedPCMFrames(std::vector<uint8_t> &data) = 0;
};

#endif