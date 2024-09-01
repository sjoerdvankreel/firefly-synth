#pragma once

#include <cstdint>

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

// once per block output data
// we send back from audio->gui
// so it can paint fancy stuff
// it was at one point modulation output only
// but now also voice start/end 
// but i dont feel like renaming a gazillion occurrences
// also it must fit in sizeof(double) to simplify the
// vst3 implementation

enum output_event_type
{ 
  out_event_voice_activation,
  out_event_cv_state,
  out_event_param_state
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

union mod_out_state 
{
  mod_out_cv_state cv;
  mod_out_voice_state voice;
  mod_out_param_state param;
};

union modulation_output 
{
  mod_out_state state;
  std::uint64_t packed;

  static modulation_output 
  make_mod_out_voice_state(std::uint8_t voice_index, bool is_active, std::uint32_t stream_time_low)
  {
    modulation_output result;
    result.state.voice.is_active = is_active;
    result.state.voice.voice_index = voice_index;
    result.state.voice.stream_time_low = stream_time_low;
    result.state.voice.event_type = out_event_voice_activation;
    return result;
  }

  static modulation_output
  make_mod_output_cv_state(std::int8_t voice_index, std::uint8_t module_global, float position_normalized)
  {
    assert(voice_index >= -1);
    assert(-1e-5 <= position_normalized && position_normalized <= 1 + 1e-5);
    modulation_output result;
    result.state.cv.voice_index = voice_index;
    result.state.cv.module_global = module_global;
    result.state.cv.event_type = out_event_cv_state;
    result.state.cv.position_normalized = position_normalized;
    return result;
  }
  
  static modulation_output
  make_mod_output_param_state(std::int8_t voice_index, std::uint8_t module_global, std::uint16_t param_global, float value_normalized)
  {
    assert(voice_index >= -1); 
    assert(-1e-5 <= value_normalized && value_normalized <= 1 + 1e-5);
    modulation_output result;
    result.state.param.voice_index = voice_index;
    result.state.param.param_global = param_global;
    result.state.param.module_global = module_global;
    result.state.param.event_type = out_event_param_state;
    result.state.param.value_normalized = (std::uint16_t)(std::clamp(value_normalized, 0.0f, 1.0f) * std::numeric_limits<std::uint16_t>::max());
    return result;
  }
};

}