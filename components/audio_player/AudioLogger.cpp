
#include "AudioLogger.h"
namespace esphome {
namespace audio_player {

DevNullOut silencedLogger;
Print* audioLogger = &silencedLogger;

}  // namespace audio_player
}  // namespace esphome