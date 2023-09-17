#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_common.hpp>

#include <vector>

namespace plugin_base {

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

// these are translated to curves/values
struct host_events final {
  std::vector<block_event> out;
  std::vector<block_event> block;
  std::vector<accurate_event> accurate;
  INF_DECLARE_MOVE_ONLY(host_events);
};

// shared block, automation events
struct host_block final {
  host_events events;
  common_block* common;
  float* const* audio_out;
  INF_DECLARE_MOVE_ONLY(host_block);
};

}