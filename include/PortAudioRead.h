#include <vector>
#include <string>

#include <portaudio.h>
#include <sndfile.h>

struct Playback
{
  SNDFILE* audioFile;
  int position;
};

class PortAudioRead
{
public:
  PortAudioRead(SNDFILE* audioFile, int num_frames, int num_channels) throw(std::string);
  ~PortAudioRead();

  void Start();

  static int Callback(const void *input,
                      void *output,
                      unsigned long fame_count,
                      const PaStreamCallbackTimeInfo *time_info,
                      PaStreamCallbackFlags status_flags,
                      void *user_data);

private:
  const int CHANNEL_COUNT = 2;
  const int SAMPLE_RATE = 44000;
  const PaStreamParameters *NO_INPUT = nullptr;
  
  int num_frames;
  int num_channels;
  PaStream* pa_stream;
  SNDFILE* audioFile;
  int position = 0;
};