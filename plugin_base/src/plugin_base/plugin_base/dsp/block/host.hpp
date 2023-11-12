#pragma once

#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/dsp/block/shared.hpp>

#include <vector>

namespace plugin_base {

enum class note_event_type { on, off, cut };
  
// once per block automation
struct block_event final {
  int param;
  normalized_value normalized;
};

// midi automation
struct midi_event final {
  // midi_source_id
  int id;
  int frame;
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
  note_event_type type;
};

// these are translated to curves/values
struct host_events final {
  std::vector<midi_event> midi;
  std::vector<block_event> out;
  std::vector<note_event> notes;
  std::vector<block_event> block;
  std::vector<accurate_event> accurate;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(host_events);
};

// shared block, note/automation events, host audio out
struct host_block final {
  int frame_count;
  host_events events;
  shared_block shared;
  float* const* audio_out;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(host_block);
};

}