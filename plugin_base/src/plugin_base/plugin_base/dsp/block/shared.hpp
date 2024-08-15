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

struct custom_out_state_data final {
  std::uint8_t module;
  std::uint8_t module_slot;
  std::uint8_t param;
  std::uint8_t param_slot;
  std::uint8_t voice;
  std::uint8_t user;
  std::uint16_t value;
};

// can be anything, but currently used for visual modulation indication
// note this is really state not event-stream! pushed once per-block, cleared on each round
union custom_out_state final {
  std::uint64_t packed;
  custom_out_state_data data;
};

}