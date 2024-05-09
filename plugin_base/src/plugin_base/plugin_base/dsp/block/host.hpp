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
  bool is_mod; // is_mod = clap nondestructive
  double value_or_offset; // [0, 1] automation value or [-1, 1] mod value
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
  std::vector<accurate_event> accurate_automation;
  std::vector<accurate_event> accurate_modulation;

  // plugin_engine interpolates these as one
  std::vector<accurate_event> accurate_automation_and_modulation;

  void deactivate();
  void activate(bool graph, int param_count, int midi_count, int polyphony, int max_frame_count);
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(host_events);
};

// shared block, note/automation events, host audio out
struct host_block final {
  int frame_count;
  host_events events;
  shared_block shared;
  float* const* audio_out;

  void prepare();
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(host_block);
};

}