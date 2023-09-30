#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_common.hpp>

#include <vector>

namespace plugin_base {

// special purpose messages send as block events to the engine
enum class engine_messages {
  first = std::numeric_limits<int>::max() - 100,
  ui_state_check = first
};
  
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

// keyboard event
struct note_event final {
  int frame;
  note_id id;
  float velocity;
  enum class type_t { on, off, cut } type;
};

// these are translated to curves/values
struct host_events final {
  std::vector<block_event> out;
  std::vector<note_event> notes;
  std::vector<block_event> block;
  std::vector<accurate_event> accurate;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(host_events);
};

// shared block, note/automation events, host audio out
struct host_block final {
  int frame_count;
  host_events events;
  common_block common;
  float* const* audio_out;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(host_block);
};

}