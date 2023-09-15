#pragma once

#include <plugin_base/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  short key;
  short channel;
};

// keyboard event
struct note_event final {
  int frame;
  note_id id;
  float velocity;
  enum class type { on, off, cut } type;
};

// passed from host to plug
struct common_block final {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  std::vector<note_event> notes;
  float* const* audio_output;
  float const* const* audio_input;
  INF_DECLARE_MOVE_ONLY(common_block);
};

}
