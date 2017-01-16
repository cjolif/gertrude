/**
 * author: Christophe Jolif (cjolif@gmail.com)
 *
 * Apache License
 **/

// Include code excrept from snowboy under Apache License and libb64 under Creative Commons public domain

// example/C++/demo.cc

// Copyright 2016  KITT.AI (author: Guoguo Chen)

/*
cencoder.c - c source to a base64 encoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include <csignal>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

#include <sndfile.h>
#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>

#include "include/snowboy-detect.h"

#include "include/PortAudioRead.h"
#include "include/PortAudioWrite.h"

// start - libb64

typedef enum {
  step_A,
  step_B,
  step_C
} base64_encodestep;

typedef struct
{
  base64_encodestep step;
  char result;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate *state_in)
{
  state_in->step = step_A;
  state_in->result = 0;
}

char base64_encode_value(char value_in)
{
  static const char *encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (value_in > 63)
  {
    printf("got incorrect value during base64 encoding\n");
    return '=';
  }
  return encoding[(int)value_in];
}

int base64_encode_block(const char *plaintext_in, int length_in, char *code_out, base64_encodestate *state_in)
{
  const char *plainchar = plaintext_in;
  const char *const plaintextend = plaintext_in + length_in;
  char *codechar = code_out;
  char result;
  char fragment;

  result = state_in->result;

  switch (state_in->step)
  {
    while (1)
    {
    case step_A:
      if (plainchar == plaintextend)
      {
        state_in->result = result;
        state_in->step = step_A;
        return codechar - code_out;
      }
      fragment = *plainchar++;
      result = (fragment & 0x0fc) >> 2;
      *codechar++ = base64_encode_value(result);
      result = (fragment & 0x003) << 4;
    case step_B:
      if (plainchar == plaintextend)
      {
        state_in->result = result;
        state_in->step = step_B;
        return codechar - code_out;
      }
      fragment = *plainchar++;
      result |= (fragment & 0x0f0) >> 4;
      *codechar++ = base64_encode_value(result);
      result = (fragment & 0x00f) << 2;
    case step_C:
      if (plainchar == plaintextend)
      {
        state_in->result = result;
        state_in->step = step_C;
        return codechar - code_out;
      }
      fragment = *plainchar++;
      result |= (fragment & 0x0c0) >> 6;
      *codechar++ = base64_encode_value(result);
      result = (fragment & 0x03f) >> 0;
      *codechar++ = base64_encode_value(result);
    }
  }
  /* control should not reach here */
  return codechar - code_out;
}

int base64_encode_blockend(char *code_out, base64_encodestate *state_in)
{
  char *codechar = code_out;

  switch (state_in->step)
  {
  case step_B:
    *codechar++ = base64_encode_value(state_in->result);
    *codechar++ = '=';
    *codechar++ = '=';
    break;
  case step_C:
    *codechar++ = base64_encode_value(state_in->result);
    *codechar++ = '=';
    break;
  case step_A:
    break;
  }

  return codechar - code_out;
}

// stop - libb64

void OpenStream(FILE **stream, char **buf, size_t *len, SF_INFO *sfinfo, SNDFILE **outfile)
{
#ifdef HAVE_POSIX_MEMSTREAM
  *stream = open_memstream(buf, len);
#else
  *stream = fopen("/tmp/sound.wav", "wb+"); //tmpfile();
#endif
  if (*stream == NULL)
    exit(1);

  if (!(*outfile = sf_open_fd(fileno(*stream), SFM_WRITE, sfinfo, 1)))
  {
    std::cerr << "Error: could not open /tmp/sound.was" << std::endl;
    puts(sf_strerror(NULL));
    exit(1);
  }
}

struct command_details {
  std::vector<std::string> details;
  std::string command;
};

std::map<std::string, std::vector<command_details*>*> commands;

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = static_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
  if (mem->memory == NULL)
  {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  std::memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void RecognizeSpeech(int rate, char *buf, size_t len)
{
  CURL *curl;
  CURLcode res;
  char *base64;

  struct MemoryStruct chunk;

  chunk.memory = static_cast<char *>(malloc(1)); /* will be grown as needed by the realloc above */
  chunk.size = 0;                                /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* get a curl handle */
  curl = curl_easy_init();
  if (curl)
  {
    /* First set the URL that is about to receive our POST. This URL can
                      just as well be a https:// URL if that is what should receive the
                      data. */
    curl_easy_setopt(curl, CURLOPT_URL, "https://speech.googleapis.com/v1beta1/speech:syncrecognize?key=AIzaSyAPm11TJPz02YsL3KqgcmuEwEtjo_1uBPM");
    /* Now specify the POST data */
    base64_encodestate es;
    base64 = static_cast<char *>(malloc(2 * len));
    /* keep track of our encoded position */
    char *c = base64;
    /* store the number of bytes encoded by a single call */
    int cnt = 0;

    base64_init_encodestate(&es);
    cnt = base64_encode_block(buf, len, base64, &es);
    c += cnt;
    cnt = base64_encode_blockend(base64 + cnt, &es);
    c += cnt;
    *c = 0;
    json_error_t error;
    json_t *json = json_pack_ex(&error, 0, "{s{s:s,s:i,s:s}s{s:s}}", "config", "encoding", "LINEAR16",
                                "sampleRate", rate, "languageCode", "fr-FR", "audio", "content", base64);
    if (!json)
    {
      printf("error: %s\n", error.text);
    }
    free(base64);
    char *jsonstr = json_dumps(json, JSON_ENCODE_ANY);
    char *header = static_cast<char *>(malloc(50));
    sprintf(header, "Content-Length: %zu", std::strlen(jsonstr));
    struct curl_slist *list = curl_slist_append(NULL, header);
    list = curl_slist_append(list, "Content-Type: application/json");
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonstr);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    else
    {
      json_error_t error;
      json_t *json = json_loads(chunk.memory, 0, &error);
      if (json == NULL)
      {
        printf("error %s\n", error.text);
      }
      const char *phrase;
      double confidence;
      json_unpack(json, "{s:[{s:[{s:s,s:f}]}]}", "results", "alternatives", "transcript", &phrase, "confidence", &confidence);
      printf("phrase: %s\n", phrase);
     
      std::string ref(phrase);
      std::vector<std::string> phrase_tokens;
      std::istringstream iss(ref);
      std::string token;

      while (std::getline(iss, token, ' '))
      {
        phrase_tokens.push_back(token);
      }

      if (commands[phrase_tokens[0]] != NULL) {
        std::cout << "we have a candidate" << std::endl;
        std::vector<command_details*> possible_commands = *commands[phrase_tokens[0]];
        std::pair<int, int> best_command(-1, 0);
        phrase_tokens.erase(phrase_tokens.begin());
        std::sort(phrase_tokens.begin(), phrase_tokens.end());
        int score, count = 0;

        for (auto &value : possible_commands) {
          std::cout << value->details[0] << std::endl;
          std::sort(value->details.begin(), value->details.end());
          std::vector<std::string> result(value->details.size());
          std::vector<std::string>::iterator it =
            std::set_intersection(phrase_tokens.begin(), phrase_tokens.end(),
              value->details.begin(), value->details.end(), result.begin());
          score = it - result.begin();
          std::cout << "interesaction is " << score << std::endl;
          if (score > best_command.second) {
            best_command.first = count;
            best_command.second = score;
          }
          count++;
        }

        if (best_command.first > -1) {
          std::string command = possible_commands[best_command.first]->command;
          std::cout << "starting " << command << std::endl;
          system(command.c_str());
        } else {
          std::cout << "no command was found" << std::endl;
        }
      } else {
        std::cout << "no command was found" << std::endl;
      }

      free(header);
    }

    free(chunk.memory);

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

#ifndef HAVE_POSIX_MEMSTREAM
void readSoundFile(FILE *file, char **code, size_t *n)
{
  int c;
  size_t s = 0;
  char *r;

  if (file == NULL)
    return; //could not open file
  fseek(file, 0, SEEK_END);
  if (ferror(file))
    printf("Error fseek1\n");
  long f_size = ftell(file);
  if (ferror(file))
    printf("Error ftell\n");
  fseek(file, 0, SEEK_SET);
  if (ferror(file))
    printf("Error fseek2\n");
  r = (char *)malloc(f_size);

  while ((c = fgetc(file)) != EOF)
  {
    r[s++] = (char)c;
  }

  if (ferror(file))
    printf("Error fgetc\n");

  r[s] = '\0';
  *code = r;
  *n = s;
  printf("file size %lu\n", *n);
}
#endif

PortAudioRead* InitDong()
{
   SF_INFO sf_info;
   SNDFILE *snd_file = sf_open("./resources/dong.wav", SFM_READ, &sf_info);
   if (snd_file == NULL) {
     std::cerr << "Dong file is not available" << std::endl;
     return NULL;
   } else {
    return new PortAudioRead(snd_file, sf_info.frames, sf_info.channels) ;
   }
}

void InitCommands()
{
    std::cout << "Initializeing commands" << std::endl;
    std::ifstream file("./resources/commands.txt");
    std::string str;
    std::string token;
    char delim;
    int i;

    while (std::getline(file, str))
    {
      std::istringstream iss(str);
      i = 0;
      delim = ',';
      command_details* details;
      while (std::getline(iss, token, delim))
      {
        switch(i) {
          case 0:
            // we have a new command line, let's create details for it
            details = new command_details;
            // if its first token is not already in the index, we create a vector entry for it
            if (commands[token] == NULL) {
              commands[token] = new std::vector<command_details*>;
            }
            // we store the (empty) details in the index
            commands[token]->push_back(details);
            delim = '|';
            break;
          case 4:
            // we are done we store the command
            details->command = token;
            break;
          case 2:
            delim = '=';
          default:
            // we store the token in the details
            if (!token.empty()) {
              details->details.push_back(token);
            }
        }
        i++;
      }
    }
}

int main(int argc, char *argv[])
{
  std::cout << "Starting initialization" << std::endl;

  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  // Parameter section.
  // If you have multiple hotword models (e.g., 2), you should set
  // <model_filename> and <sensitivity_str> as follows:
  //   model_filename = "resources/snowboy.umdl,resources/alexa.pmdl";
  //   sensitivity_str = "0.4,0.4";
  std::string resource_filename = "resources/common.res";
  std::string model_filename = "resources/Gertrude.pmdl";
  std::string sensitivity_str = "0.5";
  float audio_gain = 2;

  PortAudioRead* pa_read = InitDong();

  InitCommands();

  // Initializes Snowboy detector.
  snowboy::SnowboyDetect detector(resource_filename, model_filename);
  detector.SetSensitivity(sensitivity_str);
  detector.SetAudioGain(audio_gain);

  // Initializes PortAudio. You may use other tools to capture the audio.
  PortAudioWrite pa_write(detector.SampleRate(), detector.NumChannels(), detector.BitsPerSample());

  // Runs the detection.
  // Note: I hard-coded <int16_t> as data type because detector.BitsPerSample()
  //       returns 16.
  std::cout << "Listening... Press Ctrl+C to exit" << std::endl;
  std::vector<int16_t> data;
  SF_INFO sfinfo;
  bool saveVoice = false;
  std::memset(&sfinfo, 0, sizeof(sfinfo));
  sfinfo.samplerate = detector.SampleRate();
  sfinfo.channels = detector.NumChannels();
  sfinfo.format = (SF_FORMAT_RAW | SF_FORMAT_PCM_16);

  char *buf;
  size_t len;
  FILE *stream;
  SNDFILE *outfile;
  long silence = 0;
  bool isBufAllocated = false;

  OpenStream(&stream, &buf, &len, &sfinfo, &outfile);
#ifdef HAVE_POSIX_MEMSTREAM   
  isBufAllocated = true;
#endif

  while (true)
  {
    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, SIGINT))
    {
      sf_close(outfile);
      if (isBufAllocated)
        free(buf);
      raise(SIGINT);
      sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    }
    pa_write.Start(&data);
    if (data.size() != 0)
    {
      if (saveVoice)
      {
        sf_write_short(outfile, data.data(), data.size());
      }
      int result = detector.RunDetection(data.data(), data.size());
      if (!saveVoice && result > 0)
      {
        std::cout << "Hotword " << result << " detected!" << std::endl;
        saveVoice = true;
      }
      else if (saveVoice && result == -2)
      {
        if (silence < 10)
        {
          silence++;
        }
        else
        {
          silence = 0;
          saveVoice = false;
#ifndef HAVE_POSIX_MEMSTREAM
          readSoundFile(stream, &buf, &len);
          isBufAllocated = true;
#endif
          sf_close(outfile);
          // let the user know we are sending something to Google
          std::cout << "sending information to Google" << std::endl;
          pa_read->Start();
          RecognizeSpeech(detector.SampleRate(), buf, len);
          free(buf);
          isBufAllocated = false;
          OpenStream(&stream, &buf, &len, &sfinfo, &outfile);
        }
      }
    }
  }

  return 0;
}
