#include <iostream>
#include <string>
#include <memory>
#include <PlainConnection.h>
#include <Session.h>
#include "SpotifyTrack.h"
#include <authentication.pb.h>
#include <MercuryManager.h>
#include <unistd.h>
#include <fstream>
extern "C"
{

#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c" // Enables Vorbis decoding.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    ma_decoder *pDecoder = reinterpret_cast<ma_decoder *>(pDevice->pUserData);
    if (pDecoder == NULL)
    {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);

    // (void)pInput;
}

size_t readCallback(ma_decoder *pDecoder, void *pBufferOut, size_t bytesToRead)
{
    auto track = reinterpret_cast<SpotifyTrack *>(pDecoder->pUserData);
    auto data = track->read(bytesToRead);
    printf("I got data btw\n");
    memcpy(pBufferOut, data.data(), data.size());

    return data.size();
}

ma_bool32 seekCallback(ma_decoder *pDecoder, int byteOffset, ma_seek_origin origin)
{
    printf("Tried to seek\n");
    return false;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "usage:\n";
        std::cout << "    cspot <username> <password>\n";
        return 1;
    }

    auto connection = std::make_shared<PlainConnection>();
    connection->connectToAp();

    auto session = std::make_unique<Session>();
    session->connect(connection);
    auto token = session->authenticate(std::string(argv[1]), std::string(argv[2]));

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(session->shanConn);
        mercuryManager->startTask();
        // usleep(3000000);
        auto track = new SpotifyTrack(mercuryManager);
        while (track->chunkBuffer.size() == 0);
        // auto oggFile = std::ofstream("dodo.ogg", std::ios_base::out | std::ios_base::binary);
        auto written = 0;

        ma_result result;
        ma_decoder decoder;
        ma_device_config deviceConfig;
        ma_device device;

        result = ma_decoder_init_vorbis(readCallback, seekCallback, track, NULL, &decoder);
        if (result != MA_SUCCESS)
        {
            printf("Yeah\n");
            return -2;
        }

        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format = decoder.outputFormat;
        deviceConfig.playback.channels = 2;
        deviceConfig.sampleRate = decoder.outputSampleRate;
        deviceConfig.dataCallback = data_callback;
        deviceConfig.pUserData = &decoder;

        if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
        {
            printf("Failed to open playback device.\n");
            ma_decoder_uninit(&decoder);
            return -3;
        }

        if (ma_device_start(&device) != MA_SUCCESS)
        {
            printf("Failed to start playback device.\n");
            ma_device_uninit(&device);
            ma_decoder_uninit(&decoder);
            return -4;
        }

        printf("Press Enter to quit...");
        getchar();

        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }
    return 0;
}