#pragma once

#include <plugin_base/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

enum class note_event_type { on, off, cut };

// note id or PCK (port is 0)
struct note_id final {
  int id;
  short key;
  short channel;
};

// keyboard event
struct note_event final {
  note_id id;
  float velocity;
  int frame_index;
  note_event_type type;
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
