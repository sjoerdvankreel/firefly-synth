#pragma once

#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  short key;
  short channel;
};

// shared host/plug
struct common_block final {
  float bpm;
  float const* const* audio_in;
};

// for polyphonic stuff
enum class voice_stage { inactive, active, release };
struct voice_state final {
  note_id id = {};
  int end_frame = -1;
  int start_frame = -1;
  float velocity = 0.0f;
  std::int64_t time = -1;
  voice_stage stage = {};
};

}