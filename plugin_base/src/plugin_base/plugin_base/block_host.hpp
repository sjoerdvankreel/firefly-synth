#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace infernal::base {

struct common_block;

// once per block automation
struct host_block_event final {
  double normalized;
  int plugin_param_index;
};

// sample accurate automation
struct host_accurate_event final {
  double normalized;
  int frame_index;
  int plugin_param_index;
};

// output buffers, shared block, automation events
struct host_block final {
  common_block* common;
  float* const* audio_output;
  std::vector<host_block_event> block_events;
  std::vector<host_accurate_event> accurate_events;
  INF_DECLARE_MOVE_ONLY(host_block);
};

}
