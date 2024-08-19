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

struct mod_indicator_state_data final {
  std::uint8_t module;
  std::uint8_t module_slot;
  std::uint8_t voice;
  std::uint8_t user;
  float value;
};

// visual modulation indication
union mod_indicator_state final {
  std::uint64_t packed;
  mod_indicator_state_data data;
};

}