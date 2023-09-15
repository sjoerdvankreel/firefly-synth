#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <vector>

namespace plugin_base {

struct common_block;

// once per block automation
struct host_block_event final {
  int param_global_index;
  normalized_value normalized;
};

// sample accurate automation
struct host_accurate_event final {
  int frame_index;
  int param_global_index;
  normalized_value normalized;
};

// shared block, automation events
struct host_block final {
  common_block* common;
  std::vector<host_block_event> block_events;
  std::vector<host_block_event> output_events;
  std::vector<host_accurate_event> accurate_events;
  INF_DECLARE_MOVE_ONLY(host_block);
};

}
