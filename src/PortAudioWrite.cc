#include "PortAudioWrite.h"

#include <pa_util.h>

PortAudioWrite::PortAudioWrite(int sample_rate, int num_channels, int bits_per_sample)
{
  num_lost_samples_ = 0;
  min_read_samples_ = sample_rate * 0.1;
  Init(sample_rate, num_channels, bits_per_sample);
}

PortAudioWrite::~PortAudioWrite()
{
  Pa_StopStream(pa_stream_);
  Pa_CloseStream(pa_stream_);
  Pa_Terminate();
  PaUtil_FreeMemory(ringbuffer_);
}

int PortAudioWrite::Callback(const void *input, void *output,
                             unsigned long frame_count,
                             const PaStreamCallbackTimeInfo *time_info,
                             PaStreamCallbackFlags status_flags, void *user_data)
{
  // Input audio.
  PortAudioWrite *handler = static_cast<PortAudioWrite *>(user_data);

  ring_buffer_size_t num_written_samples =
      PaUtil_WriteRingBuffer(&handler->pa_ringbuffer_, input, frame_count);
  handler->num_lost_samples_ += frame_count - num_written_samples;
  return paContinue;
}

bool PortAudioWrite::Init(int sample_rate, int num_channels, int bits_per_sample)
{
  // Allocates ring buffer memory.
  int ringbuffer_size = 16384;
  ringbuffer_ = static_cast<char *>(
      PaUtil_AllocateMemory(bits_per_sample / 8 * ringbuffer_size));
  if (ringbuffer_ == NULL)
  {
    std::cerr << "Fail to allocate memory for ring buffer." << std::endl;
    return false;
  }

  // Initializes PortAudio ring buffer.
  ring_buffer_size_t rb_init_ans =
      PaUtil_InitializeRingBuffer(&pa_ringbuffer_, bits_per_sample / 8,
                                  ringbuffer_size, ringbuffer_);
  if (rb_init_ans == -1)
  {
    std::cerr << "Ring buffer size is not power of 2." << std::endl;
    return false;
  }

  // Initializes PortAudio.
  PaError pa_init_ans = Pa_Initialize();
  if (pa_init_ans != paNoError)
  {
    std::cerr << "Fail to initialize PortAudio, error message is \""
              << Pa_GetErrorText(pa_init_ans) << "\"" << std::endl;
    return false;
  }

  PaError pa_open_ans;
  if (bits_per_sample == 8)
  {
    pa_open_ans = Pa_OpenDefaultStream(
        &pa_stream_, num_channels, 0, paUInt8, sample_rate,
        paFramesPerBufferUnspecified, &Callback, this);
  }
  else if (bits_per_sample == 16)
  {
    pa_open_ans = Pa_OpenDefaultStream(
        &pa_stream_, num_channels, 0, paInt16, sample_rate,
        paFramesPerBufferUnspecified, &Callback, this);
  }
  else if (bits_per_sample == 32)
  {
    pa_open_ans = Pa_OpenDefaultStream(
        &pa_stream_, num_channels, 0, paInt32, sample_rate,
        paFramesPerBufferUnspecified, &Callback, this);
  }
  else
  {
    std::cerr << "Unsupported BitsPerSample: " << bits_per_sample
              << std::endl;
    return false;
  }
  if (pa_open_ans != paNoError)
  {
    std::cerr << "Fail to open PortAudio stream, error message is \""
              << Pa_GetErrorText(pa_open_ans) << "\"" << std::endl;
    return false;
  }

  PaError pa_stream_start_ans = Pa_StartStream(pa_stream_);
  if (pa_stream_start_ans != paNoError)
  {
    std::cerr << "Fail to start PortAudio stream, error message is \""
              << Pa_GetErrorText(pa_stream_start_ans) << "\"" << std::endl;
    return false;
  }
  return true;
}
