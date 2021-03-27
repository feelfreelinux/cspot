#include "BufferedAudioSink.h"

#include "driver/i2s.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

RingbufHandle_t dataBuffer;

static void i2sFeed(void *pvParameters)
{
    while (true)
    {
        size_t itemSize;
        char *item = (char *)xRingbufferReceiveUpTo(dataBuffer, &itemSize, portMAX_DELAY, 512);
        if (item != NULL)
        {
            size_t written = 0;
            while (written < itemSize)
            {
                i2s_write((i2s_port_t)0, item, itemSize, &written, portMAX_DELAY);
            }
            vRingbufferReturnItem(dataBuffer, (void *)item);
        }
    }
}

void BufferedAudioSink::startI2sFeed(size_t buf_size)
{    
    dataBuffer = xRingbufferCreate(buf_size, RINGBUF_TYPE_BYTEBUF);
    xTaskCreatePinnedToCore(&i2sFeed, "i2sFeed", 4096, NULL, 10, NULL, tskNO_AFFINITY);
}

void BufferedAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    feedPCMFramesInternal(&data[0], data.size());
}

void BufferedAudioSink::feedPCMFramesInternal(const void *pvItem, size_t xItemSize)
{
    xRingbufferSend(dataBuffer, pvItem, xItemSize, portMAX_DELAY);
}
