#include <vector>
#include <cassert>
#include <iostream>

#include <portaudio.h>
#include <pa_ringbuffer.h>

class PortAudioWrite
{
public:
  // Constructor.
  PortAudioWrite(int sample_rate, int num_channels, int bits_per_sample);
  ~PortAudioWrite();

  // Reads data from ring buffer.
  // Reads data from ring buffer.
  template <typename T>
  void Start(std::vector<T> *data)
  {
    assert(data != NULL);

    // Checks ring buffer overflow.
    if (num_lost_samples_ > 0)
    {
      std::cerr << "Lost " << num_lost_samples_ << " samples due to ring"
                << " buffer overflow." << std::endl;
      num_lost_samples_ = 0;
    }

    ring_buffer_size_t num_available_samples = 0;
    while (true)
    {
      num_available_samples =
          PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
      if (num_available_samples >= min_read_samples_)
      {
        break;
      }
      Pa_Sleep(5);
    }

    // Reads data.
    num_available_samples = PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
    data->resize(num_available_samples);
    ring_buffer_size_t num_read_samples = PaUtil_ReadRingBuffer(
        &pa_ringbuffer_, data->data(), num_available_samples);
    if (num_read_samples != num_available_samples)
    {
      std::cerr << num_available_samples << " samples were available,  but "
                << "only " << num_read_samples << " samples were read." << std::endl;
    }
  }

  static int Callback(const void *input, void *output,
                      unsigned long frame_count,
                      const PaStreamCallbackTimeInfo *time_info,
                      PaStreamCallbackFlags status_flags, void *user_data);

private:
  // Initialization.
  bool Init(int sample_rate, int num_channels, int bits_per_sample);
  // Pointer to the ring buffer memory.
  char *ringbuffer_;
  // Ring buffer wrapper used in PortAudio.
  PaUtilRingBuffer pa_ringbuffer_;
  // Pointer to PortAudio stream.
  PaStream *pa_stream_;
  // Number of lost samples at each Start() due to ring buffer overflow.
  int num_lost_samples_;
  // Wait for this number of samples in each Start() call.
  int min_read_samples_;
};
