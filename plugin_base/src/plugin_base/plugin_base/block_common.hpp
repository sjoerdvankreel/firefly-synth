#pragma once

#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  short key;
  short channel;
};

// for polyphonic stuff
// TODO start/end frames
struct voice_state final {
  note_id id = {};
  bool active = false;
  float velocity = 0.0f;
  std::int64_t time = -1;
};

// shared host/plug
struct common_block final {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  float const* const* audio_in;
};

}