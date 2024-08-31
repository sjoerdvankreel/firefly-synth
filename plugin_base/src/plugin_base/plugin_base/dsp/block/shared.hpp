#pragma once

#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  int key;
  int channel;
};

// shared host/plug
struct shared_block final {
  float bpm;
  float const* const* audio_in;
};

// once per block output data
// we send back from audio->gui
// so it can paint fancy stuff
struct modulation_output_data final {
  std::uint8_t voice_index;
  std::uint8_t module_global;
  std::int16_t param_global;
  float value;
};

union modulation_output final {
  std::uint64_t packed;
  modulation_output_data data;
};

}