#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <vector>

namespace plugin_base {

struct common_block;

// once per block automation
struct block_event final {
  int param;
  normalized_value normalized;
};

// sample accurate automation
struct accurate_event final {
  int frame;
  int param;
  normalized_value normalized;
};

// shared block, automation events
struct host_block final {
  common_block* common;
  std::vector<block_event> out_events;
  std::vector<block_event> block_events;
  std::vector<accurate_event> accurate_events;
  INF_DECLARE_MOVE_ONLY(host_block);
};

}
