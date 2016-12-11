#include "PortAudioRead.h"

#include <iostream>
#include <sstream>
#include <cstring>

PortAudioRead::PortAudioRead(SNDFILE *audioFile, int num_frames, int num_channels) throw(std::string)
    : audioFile(audioFile), num_frames(num_frames), num_channels(num_channels) 
{
    Pa_Initialize();
    PaError errorCode;
    PaStreamParameters outputParameters;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = CHANNEL_COUNT;
    outputParameters.sampleFormat = paInt32;
    outputParameters.suggestedLatency = 0.01;
    outputParameters.hostApiSpecificStreamInfo = 0;

    errorCode = Pa_OpenStream(&pa_stream,
                              NO_INPUT,
                              &outputParameters,
                              SAMPLE_RATE,
                              paFramesPerBufferUnspecified,
                              paNoFlag,
                              &Callback,
                              this);

    if (errorCode)
    {
        Pa_Terminate();

        std::stringstream error;
        error << "Unable to open stream for output. Portaudio error code: " << errorCode;
        throw error.str();
    }
}

PortAudioRead::~PortAudioRead()
{
    Pa_CloseStream(pa_stream);
    Pa_Terminate();
    sf_close(audioFile);
}

int PortAudioRead::Callback(const void *input,
                            void *output,
                            unsigned long frame_count,
                            const PaStreamCallbackTimeInfo *time_info,
                            PaStreamCallbackFlags status_flag,
                            void *user_data)
{
    PortAudioRead *handler = static_cast<PortAudioRead *>(user_data);

    unsigned long stereoFrameCount = frame_count * handler->CHANNEL_COUNT;
    std::memset((int *)output, 0, stereoFrameCount * sizeof(int));

    int *outputBuffer = new int[stereoFrameCount];
    int *bufferCursor = outputBuffer;

    unsigned int framesLeft = (unsigned int)frame_count;
    unsigned int framesRead;
    bool playbackEnded;

    while (framesLeft > 0)
    {
        sf_seek(handler->audioFile, handler->position, SEEK_SET);

        if (framesLeft > (handler->num_frames - handler->position))
        {
            framesRead = (unsigned int)(handler->num_frames - handler->position);
            playbackEnded = true;
            framesLeft = framesRead;
        }
        else
        {
            framesRead = framesLeft;
            handler->position += framesRead;
        }

        sf_readf_int(handler->audioFile, bufferCursor, framesRead);

        bufferCursor += framesRead;

        framesLeft -= framesRead;
    }

    int *outputCursor = (int *)output;
    if (handler->num_channels == 1)
    {
        for (unsigned long i = 0; i < stereoFrameCount; ++i)
        {
            *outputCursor += (0.5 * outputBuffer[i]);
            ++outputCursor;
            *outputCursor += (0.5 * outputBuffer[i]);
            ++outputCursor;
        }
    }
    else
    {
        for (unsigned long i = 0; i < stereoFrameCount; ++i)
        {
            *outputCursor += (0.5 * outputBuffer[i]);
            ++outputCursor;
        }
    }

    if (playbackEnded)
    {
        std::cout << "stop" << std::endl;
        handler->position = 0;
        return paComplete;
    }

    return paContinue;
}

void PortAudioRead::Start()
{
    std::cout << "Play dong" << std::endl;
    PaError err;
    if (!Pa_IsStreamStopped(pa_stream))
    {
        std::cout << "Stop stream" << std::endl;
        err = Pa_StopStream(pa_stream);
        if (err < 0)
        {
            std::cerr << "Error" << err << std::endl;
        }
    }
    err = Pa_StartStream(pa_stream);
    if (err < 0)
    {
        std::cerr << "Error" << err << std::endl;
    }
}
