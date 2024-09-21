#pragma once

#include <limits>
#include <cassert>
#include <cstdint>
#include <algorithm>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  int key;
  int channel;
};

// shared host/plug
struct shared_block final {
  float bpm;
  float const* const* audio_in;
};

// Once per block output data we send back from audio->gui so it can paint fancy stuff.
// It was at one point modulation output only but now also voice start/end.
// But i dont feel like renaming a gazillion occurrences.
// Also it must fit in sizeof(double) to simplify the vst3 implementation.

enum output_event_type
{ 
  out_event_voice_activation,
  out_event_cv_state,
  out_event_param_state,
  out_event_custom_state
};

struct mod_out_voice_state
{
  std::uint8_t event_type;
  std::int8_t voice_index;
  bool is_active;
  std::uint8_t padding;
  std::uint32_t stream_time_low; // assumption is notes dont go more apart than maxuint frames
};

struct mod_out_cv_state
{
  std::uint8_t event_type;
  std::int8_t voice_index; // -1 for global
  std::uint8_t module_global;
  std::uint8_t padding;
  float position_normalized;
};

struct mod_out_param_state
{
  std::uint8_t event_type;
  std::int8_t voice_index; // -1 for global
  std::uint8_t module_global;
  std::uint8_t padding;
  std::uint16_t param_global;
  std::uint16_t value_normalized;

  float normalized_real() const
  { return std::clamp((float)value_normalized / std::numeric_limits<std::uint16_t>::max(), 0.0f, 1.0f); }
};

struct mod_out_custom_state
{
  std::uint8_t event_type;
  std::int8_t voice_index; // -1 for global
  std::uint8_t module_global;
  std::uint8_t tag_custom;
  std::int32_t value_custom;
};

union mod_out_state 
{
  mod_out_cv_state cv;
  mod_out_voice_state voice;
  mod_out_param_state param;
  mod_out_custom_state custom;
};

union modulation_output 
{
  mod_out_state state;
  std::uint64_t packed;

  // stuff overlaps, all should match
  std::int8_t voice_index() const { return state.cv.voice_index; }
  output_event_type event_type() const { return (output_event_type)state.cv.event_type; }

  static modulation_output
  make_mod_out_voice_state(std::int8_t voice_index, bool is_active, std::uint32_t stream_time_low);
  static modulation_output
  make_mod_output_cv_state(std::int8_t voice_index, std::uint8_t module_global, float position_normalized);
  static modulation_output
  make_mod_output_custom_state(std::int8_t voice_index, std::uint8_t module_global, std::uint8_t tag_custom, std::int32_t value_custom);
  static modulation_output
  make_mod_output_param_state(std::int8_t voice_index, std::uint8_t module_global, std::uint16_t param_global, float value_normalized);
};

// with timestamp for voice, needed for sorting
struct timed_modulation_output
{
  modulation_output output;
  std::uint32_t stream_time_low;
};

bool operator <
(timed_modulation_output const& l, timed_modulation_output const& r);

}