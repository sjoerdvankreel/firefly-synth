#pragma once

#include <plugin_base/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

// shared host/plug
struct common_block final {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  float const* const* audio_in;
  INF_DECLARE_MOVE_ONLY(common_block);
};

}