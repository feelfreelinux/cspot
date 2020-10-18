#include "PortAudioSink.h"

#ifdef CSPOT_ENABLE_PORTAUDIO_SINK

PortAudioSink::PortAudioSink()
{
    Pa_Initialize();
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        printf("PortAudio: Default audio device not found!\n");
        // exit(0);
    }

    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paInt16; /* 32 bit floating point output */
    outputParameters.suggestedLatency = 0.050;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        44100,
        4096 / 4,
        paClipOff,
        NULL, // blocking api
        NULL
    );
    Pa_StartStream(stream);
}

PortAudioSink::~PortAudioSink()
{
    Pa_StopStream(stream);
    Pa_Terminate();
}

void PortAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    Pa_WriteStream(stream, &data[0], data.size() / 4);
}

#endif