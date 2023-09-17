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
  enum class type_t { on, off, cut } type;
};

// shared host/plug
struct common_block final {
  float bpm;
  int frame_count;
  float* const* audio_out;
  float const* const* audio_in;
  std::int64_t stream_time;
  std::vector<note_event> notes;
  INF_DECLARE_MOVE_ONLY(common_block);
};

}