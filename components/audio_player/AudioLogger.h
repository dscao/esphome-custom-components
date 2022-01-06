
#include <Arduino.h>

#pragma once

namespace esphome {
namespace audio_player {

class DevNullOut: public Print
{
public:
    virtual size_t write(uint8_t) { return 1; }
};

extern DevNullOut silencedLogger;

// Global `audioLogger` is initialized to &silencedLogger
// It can be initialized anytime to &Serial or any other Print:: derivative instance.
extern Print* audioLogger;

}  // namespace audio_player
}  // namespace esphome
