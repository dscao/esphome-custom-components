#include "CustomAudioFileSourceHTTPStream.h"

namespace esphome {
namespace audio_player {

static const char *header[] = {"Content-Type", "Transfer-Encoding"};

CustomAudioFileSourceHTTPStream::CustomAudioFileSourceHTTPStream() {
  pos = 0;
  size = 0;
  chunked = false;
  chunked_size = 0;
  reconnectTries = 0;
  reconnectDelayMs = 100;
  saveURL[0] = 0;
}

CustomAudioFileSourceHTTPStream::CustomAudioFileSourceHTTPStream(const char *url)
    : CustomAudioFileSourceHTTPStream() {
  open(url);
}

bool CustomAudioFileSourceHTTPStream::open(const char *url) {
  pos = 0;
  size = 0;
  chunked = false;
  chunked_size = 0;
  http.begin(client, url);
  http.collectHeaders(header, 2);
  http.setReuse(true);
#ifndef USE_ESP32
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#endif
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
    return false;
  }
  size = http.getSize();
  chunked = http.header("Transfer-Encoding") == "chunked";
  strncpy(saveURL, url, sizeof(saveURL) - 1);
  saveURL[sizeof(saveURL) - 1] = 0;
  return true;
}

CustomAudioFileSourceHTTPStream::~CustomAudioFileSourceHTTPStream() { http.end(); }

uint32_t CustomAudioFileSourceHTTPStream::read(void *data, uint32_t len) {
  if (data == nullptr) {
    audioLogger->printf_P(PSTR("ERROR! CustomAudioFileSourceHTTPStream::read passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, false);
}

uint32_t CustomAudioFileSourceHTTPStream::readNonBlock(void *data, uint32_t len) {
  if (data == nullptr) {
    audioLogger->printf_P(PSTR("ERROR! CustomAudioFileSourceHTTPStream::readNonBlock passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, true);
}

uint32_t CustomAudioFileSourceHTTPStream::readInternal(void *data, uint32_t len, bool nonBlock) {
retry:
  if (!http.connected()) {
    cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
    http.end();
    for (int i = 0; i < reconnectTries; i++) {
      char buff[64];
      sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
      cb.st(STATUS_RECONNECTING, buff);
      delay(reconnectDelayMs);
      if (open(saveURL)) {
        cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
        break;
      }
    }
    if (!http.connected()) {
      cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
      return 0;
    }
  }
  if ((size > 0) && (pos >= size))
    return 0;

  WiFiClient *stream = http.getStreamPtr();

  // Can't read past EOF...
  if ((size > 0) && (len > (uint32_t) (size - pos)))
    len = size - pos;

  if (!nonBlock) {
    int start = millis();
    while ((stream->available() < (int) len) && (millis() - start < 500))
      yield();
  }

  size_t avail = stream->available();
  if (!nonBlock && !avail) {
    cb.st(STATUS_NODATA, PSTR("No stream data available"));
    http.end();
    goto retry;
  }
  if (avail == 0)
    return 0;
  if (avail < len)
    len = avail;

  int read = 0;
  if (!chunked) {
    read = stream->read(reinterpret_cast<uint8_t *>(data), len);
    pos += read;
    return read;
  }

  if (chunked_size < 0) {
    return 0;  // EOF
  }

  if (chunked_size == 0) {
    String chunkHeader = stream->readStringUntil('\n');
    if (chunkHeader.length() <= 0) {
      audioLogger->printf_P(PSTR("ERROR! CustomAudioFileSourceHTTPStream::no chunked len!"));
      return 0;
    }
    chunkHeader.trim();
    chunked_size = (int) strtol((const char *) chunkHeader.c_str(), nullptr, 16);
    if (chunked_size == 0) {
      http.end();
      chunked_size = -1;
    }
  }

  if (chunked_size > 0) {
    if (len > (uint32_t) chunked_size)
      len = (uint32_t) chunked_size;
    read = stream->read(reinterpret_cast<uint8_t *>(data), len);
    pos += read;
    chunked_size -= read;
  }

  if (chunked_size == 0) {
    char buf[2];
    auto trailing_seq_len = stream->readBytes((uint8_t *) buf, 2);
    if (trailing_seq_len != 2 || buf[0] != '\r' || buf[1] != '\n') {
      audioLogger->printf_P(PSTR("ERROR! CustomAudioFileSourceHTTPStream::chunked error!"));
      http.end();
      chunked_size = -1;
    }
  }

  return read;
}

bool CustomAudioFileSourceHTTPStream::seek(int32_t pos, int dir) {
  audioLogger->printf_P(PSTR("ERROR! CustomAudioFileSourceHTTPStream::seek not implemented!"));
  (void) pos;
  (void) dir;
  return false;
}

bool CustomAudioFileSourceHTTPStream::close() {
  http.end();
  return true;
}

bool CustomAudioFileSourceHTTPStream::isOpen() { return http.connected(); }

uint32_t CustomAudioFileSourceHTTPStream::getSize() { return size; }

uint32_t CustomAudioFileSourceHTTPStream::getPos() { return pos; }

}  // namespace audio_player
}  // namespace esphome
