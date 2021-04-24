#include "ALSAAudioSink.h"

#ifdef CSPOT_ENABLE_ALSA_SINK

ALSAAudioSink::ALSAAudioSink()
{
    unsigned int card = 0;
    unsigned int device = 0;
    int flags = PCM_OUT;

    const struct pcm_config config = {
        .channels = 2,
        .rate = 44100,
        .period_size = 1024,
        .period_count = 2,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = 1024,
        .stop_threshold = 1024 * 2,
        .silence_threshold = 1024 * 2};

    this->pcmHandle = pcm_open(card, device, flags, &config);
    if (pcmHandle == NULL)
    {
        fprintf(stderr, "failed to allocate memory for PCM\n");
    }
    else if (!pcm_is_ready(pcmHandle))
    {
        pcm_close(pcmHandle);
        fprintf(stderr, "failed to open PCM\n");
    }

    printf("required buff_size: %d\n", buff_size);
    this->startTask();
}

ALSAAudioSink::~ALSAAudioSink()
{
    pcm_close(this->pcmHandle);
}

void ALSAAudioSink::runTask()
{
    std::unique_ptr<std::vector<uint8_t>> dataPtr;
    while (true)
    {
        if (!this->ringbuffer.pop(dataPtr))
        {
            usleep(100);
            continue;
        }

        unsigned int frame_count = pcm_bytes_to_frames(pcmHandle, dataPtr->size());

        int err = pcm_writei(pcmHandle, dataPtr->data(), frame_count);
        if (err < 0)
        {
            printf("error: %s\n", pcm_get_error(pcmHandle));
        }
    }
}

void ALSAAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{

    buff.insert(buff.end(), data.begin(), data.end());
    while (buff.size() > this->buff_size)
    {
        auto ptr = std::make_unique<std::vector<uint8_t>>(this->buff.begin(), this->buff.begin() + this->buff_size);
        this->buff = std::vector<uint8_t>(this->buff.begin() + this->buff_size, this->buff.end());
        while (!this->ringbuffer.push(ptr))
        {
            usleep(100);
        };
    }
}

#endif
