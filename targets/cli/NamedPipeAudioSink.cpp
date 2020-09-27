#include "NamedPipeAudioSink.h"

NamedPipeAudioSink::NamedPipeAudioSink()
{
    this->namedPipeFile = std::ofstream("outputFifo", std::ios::binary);
}

NamedPipeAudioSink::~NamedPipeAudioSink()
{
    this->namedPipeFile.close();
}

void NamedPipeAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    // Write the actual data
    this->namedPipeFile.write((char *)&data[0], data.size());
    this->namedPipeFile.flush();
}
