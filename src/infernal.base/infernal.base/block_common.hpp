#pragma once
#include <infernal.base/utility.hpp>
#include <vector>
#include <cstdint>

namespace infernal::base {

enum class note_event_type { on, off, cut };

struct note_id final {
  int id;
  short key;
  short channel;
};

struct note_event final {
  note_id id;
  float velocity;
  int frame_index;
  note_event_type type;
};

struct common_block final {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  float* const* audio_output;
  float const* const* audio_input;
  std::vector<note_event> notes;
  INF_DECLARE_MOVE_ONLY(common_block);
};

}
