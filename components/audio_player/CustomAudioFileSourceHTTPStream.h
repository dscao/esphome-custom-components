#pragma once
#include <string>
#include <Arduino.h>
#ifdef USE_ESP32
#include <HTTPClient.h>
#else
#include <ESP8266HTTPClient.h>
#endif
#include "AudioFileSource.h"

namespace esphome {
namespace audio_player {

class CustomAudioFileSourceHTTPStream : public AudioFileSource {
  friend class AudioFileSourceICYStream;

 public:
  CustomAudioFileSourceHTTPStream();
  CustomAudioFileSourceHTTPStream(const char *url);
  ~CustomAudioFileSourceHTTPStream() override;

  bool open(const char *url) override;
  uint32_t read(void *data, uint32_t len) override;
  uint32_t readNonBlock(void *data, uint32_t len) override;
  bool seek(int32_t pos, int dir) override;
  bool close() override;
  bool isOpen() override;
  uint32_t getSize() override;
  uint32_t getPos() override;
  bool SetReconnect(int tries, int delayms) {
    reconnectTries = tries;
    reconnectDelayMs = delayms;
    return true;
  }
  void useHTTP10() { http.useHTTP10(true); }
  std::string contentType() { return std::string(http.header("Content-Type").c_str()); }

  enum { STATUS_HTTPFAIL = 2, STATUS_DISCONNECTED, STATUS_RECONNECTING, STATUS_RECONNECTED, STATUS_NODATA };

 private:
  virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
  WiFiClient client;
  HTTPClient http;
  int pos;
  int size;
  int reconnectTries;
  int reconnectDelayMs;
  char saveURL[256];
  int chunked_size;
  bool chunked;
};

}  // namespace audio_player
}  // namespace esphome
