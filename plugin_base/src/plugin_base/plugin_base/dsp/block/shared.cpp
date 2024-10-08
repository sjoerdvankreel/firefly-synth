#include <plugin_base/dsp/block/shared.hpp>

namespace plugin_base {

modulation_output
modulation_output::make_mod_out_voice_state(std::int8_t voice_index, bool is_active, std::uint32_t stream_time_low)
{
  modulation_output result;
  result.state.voice.is_active = is_active;
  result.state.voice.voice_index = voice_index;
  result.state.voice.stream_time_low = stream_time_low;
  result.state.voice.event_type = out_event_voice_activation;
  return result;
}

modulation_output
modulation_output::make_mod_output_cv_state(std::int8_t voice_index, std::uint8_t module_global, float position_normalized)
{
  assert(voice_index >= -1);
  assert(-1e-3 <= position_normalized && position_normalized <= 1 + 1e-3);
  modulation_output result;
  result.state.cv.voice_index = voice_index;
  result.state.cv.module_global = module_global;
  result.state.cv.event_type = out_event_cv_state;
  result.state.cv.position_normalized = position_normalized;
  return result;
}

modulation_output
modulation_output::make_mod_output_custom_state_int(std::int8_t voice_index, std::uint8_t module_global, std::uint8_t tag_custom, std::int32_t value_custom)
{
  assert(voice_index >= -1);
  modulation_output result;
  result.state.custom.tag_custom = tag_custom;
  result.state.custom.voice_index = voice_index;
  result.state.custom.module_global = module_global;
  result.state.custom.event_type = out_event_custom_state;
  result.state.custom.value_custom_raw = *reinterpret_cast<std::uint32_t*>(&value_custom);
  return result;
}

modulation_output
modulation_output::make_mod_output_custom_state_float(std::int8_t voice_index, std::uint8_t module_global, std::uint8_t tag_custom, float value_custom)
{
  assert(voice_index >= -1);
  modulation_output result;
  result.state.custom.tag_custom = tag_custom;
  result.state.custom.voice_index = voice_index;
  result.state.custom.module_global = module_global;
  result.state.custom.event_type = out_event_custom_state;
  result.state.custom.value_custom_raw = *reinterpret_cast<std::uint32_t*>(&value_custom);
  return result;
}
  
modulation_output
modulation_output::make_mod_output_param_state(std::int8_t voice_index, std::uint8_t module_global, std::uint16_t param_global, float value_normalized)
{
  assert(voice_index >= -1); 
  assert(-1e-3 <= value_normalized && value_normalized <= 1 + 1e-3);
  modulation_output result;
  result.state.param.voice_index = voice_index;
  result.state.param.param_global = param_global;
  result.state.param.module_global = module_global;
  result.state.param.event_type = out_event_param_state;

  // need clamp in larger datatype as 1.0f * maxuint16 can go > maxuint16 in float
  int ivalue_normalized = (int)(std::clamp(value_normalized, 0.0f, 1.0f) * std::numeric_limits<std::uint16_t>::max());
  result.state.param.value_normalized_raw = (std::uint16_t)(std::clamp(ivalue_normalized, 0, (int)std::numeric_limits<std::uint16_t>::max()));
  return result;
}

bool operator <
(timed_modulation_output const& l, timed_modulation_output const& r)
{
  if (l.output.event_type() < r.output.event_type()) return true;
  if (l.output.event_type() > r.output.event_type()) return false;
  if (l.output.event_type() == out_event_voice_activation)
  {
    // this event *is* the timestamp
    if (l.output.state.voice.voice_index < r.output.state.voice.voice_index) return true;
    if (l.output.state.voice.voice_index > r.output.state.voice.voice_index) return false;
    if (l.output.state.voice.stream_time_low < r.output.state.voice.stream_time_low) return true;
    if (l.output.state.voice.stream_time_low > r.output.state.voice.stream_time_low) return false;
    return false;
  }

  // for all others sort by timestamp first (0 for global) voice index second
  if (l.output.event_type() == out_event_cv_state)
  {
    if (l.output.state.cv.module_global < r.output.state.cv.module_global) return true;
    if (l.output.state.cv.module_global > r.output.state.cv.module_global) return false;
    if (l.stream_time_low < r.stream_time_low) return true;
    if (l.stream_time_low > r.stream_time_low) return false;
    if (l.output.state.cv.voice_index < r.output.state.cv.voice_index) return true;
    if (l.output.state.cv.voice_index > r.output.state.cv.voice_index) return false;
    return false;
  }
  if (l.output.event_type() == out_event_custom_state)
  {
    if (l.output.state.custom.module_global < r.output.state.custom.module_global) return true;
    if (l.output.state.custom.module_global > r.output.state.custom.module_global) return false;
    if (l.stream_time_low < r.stream_time_low) return true;
    if (l.stream_time_low > r.stream_time_low) return false;
    if (l.output.state.custom.voice_index < r.output.state.custom.voice_index) return true;
    if (l.output.state.custom.voice_index > r.output.state.custom.voice_index) return false;
    if (l.output.state.custom.tag_custom < r.output.state.custom.tag_custom) return true;
    if (l.output.state.custom.tag_custom > r.output.state.custom.tag_custom) return false;
    return false;
  }
  if (l.output.event_type() == out_event_param_state)
  {
    if (l.output.state.param.module_global < r.output.state.param.module_global) return true;
    if (l.output.state.param.module_global > r.output.state.param.module_global) return false;
    if (l.output.state.param.param_global < r.output.state.param.param_global) return true;
    if (l.output.state.param.param_global > r.output.state.param.param_global) return false;
    if (l.stream_time_low < r.stream_time_low) return true;
    if (l.stream_time_low > r.stream_time_low) return false;
    if (l.output.state.param.voice_index < r.output.state.param.voice_index) return true;
    if (l.output.state.param.voice_index > r.output.state.param.voice_index) return false;
    return false;
  }
  assert(false);
  return false;
}

}