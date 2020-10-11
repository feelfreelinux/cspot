#include "PortAudioSink.h"

PortAudioSink::PortAudioSink()
{
    printf("Start\n");
    this->namedPipeFile = std::ofstream("outputFifo", std::ios::binary);
    printf("stop\n");

}

PortAudioSink::~PortAudioSink()
{
    // this->namedPipeFile.close();
}

void PortAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    // Write the actual data
    // this->namedPipeFile.write((char *)&data[0], data.size());
    // this->namedPipeFile.flush();
}
