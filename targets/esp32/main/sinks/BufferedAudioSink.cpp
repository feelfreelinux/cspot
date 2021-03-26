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

void BufferedAudioSink::startI2sFeed()
{
    dataBuffer = xRingbufferCreate(4096 * 8, RINGBUF_TYPE_BYTEBUF);
    xTaskCreatePinnedToCore(&i2sFeed, "i2sFeed", 4096, NULL, 10, NULL, 1);
}

void BufferedAudioSink::feedPCMFrames(std::vector<uint8_t> &data)
{
    xRingbufferSend(dataBuffer, &data[0], data.size(), portMAX_DELAY);
}
