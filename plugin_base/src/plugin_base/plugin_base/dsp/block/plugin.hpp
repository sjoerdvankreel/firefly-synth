#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/block/shared.hpp>

#include <cassert>

namespace plugin_base {

struct plugin_topo;
enum class voice_stage { unused, active, releasing, finishing };

// for monophonic mode
struct mono_note_state
{
  int midi_key = -1;
  bool note_on = false;
};

// view of automation event stream
// which presents as a series of values
// note this class is mutable so need separate
// copies of this for per-voice threaded processing
// the underlying data itself is immutable
class sparse_buffer
{
  int const _sample_count;
  int const _point_count;
  int const* const _point_positions;
  int const* const _next_point_positions;
  double const* const _normalized_values;

  double _delta = 0;
  double _current = 0;
  int _position = 0;
  int _point_index = 0;
  int _next_point_position = 0;

  void init_section(int point_index);

public: 
  double next();
  double current() const { return _current; }

  sparse_buffer(
    int sample_count, int point_count, 
    int const* point_positions, int const* next_point_positions,
    double const* normalized_values);
};

inline sparse_buffer::
sparse_buffer(
  int sample_count, int point_count, 
  int const* point_positions, int const* next_point_positions,
  double const* normalized_values) :
_sample_count(sample_count), _point_count(point_count), 
_point_positions(point_positions), _next_point_positions(next_point_positions),
_normalized_values(normalized_values)
{
  assert(sample_count > 0);
  assert(point_count > 1); // needs first and last at the least
  assert(point_count <= sample_count);
  assert(point_positions[0] == 0);
  assert(point_positions[point_count - 1] == sample_count - 1);
  assert(next_point_positions[point_count - 1] == -1);
  for(int i = 0; i < point_count; i++)
    check_unipolar(normalized_values[i]);
  for(int i = 1; i < point_count; i++)
  {
    assert(_point_positions[i] > _point_positions[i - 1]);
    assert(_next_point_positions[i - 1] == _point_positions[i]);
  }

  init_section(0);
}

inline void
sparse_buffer::init_section(int point_index)
{
  _point_index = point_index;
  _current = _normalized_values[point_index];
  _next_point_position = _next_point_positions[point_index];
  _delta = (_normalized_values[point_index + 1] - _current) / (_next_point_position - _position);
}

inline double
sparse_buffer::next()
{
  double result = _current;
  _current += _delta;
  if(_position++ == _next_point_position) init_section(_point_index + 1);
  return result;
}

// for polyphonic synth
struct voice_state final {
  note_id note_id_ = {};
  // for mono mode
  note_id release_id = {};

  int end_frame = -1;
  int start_frame = -1;
  int release_frame = -1;
  float velocity = 0.0f;
  std::int64_t time = -1;
  voice_stage stage = {};

  // for portamento
  int last_note_key = -1;
  int last_note_channel = -1;

  // for global unison
  int sub_voice_count = -1;
  int sub_voice_index = -1;
};

// single output module process call
struct plugin_output_block final {
  int voice_count;
  int thread_count;
  double cpu_usage;
  int high_cpu_module;
  double high_cpu_module_usage;
  float* const* host_audio;
  jarray<plain_value, 2>& params;
  jarray<float, 2> const& voice_mixdown;
};

// single per-voice module process call
struct plugin_voice_block final {
  bool finished;
  jarray<float, 2>& result;
  voice_state const& state;
  jarray<float, 5> const& all_cv;
  jarray<float, 6> const& all_audio;
  jarray<void*, 2> const& all_context;
};

// state and automation
struct plugin_block_state final {
  int last_midi_note = -1;
  void** own_context = {};
  // for mono mode
  std::vector<mono_note_state> const& mono_note_stream;

  jarray<float, 3>& own_cv;
  jarray<float, 4>& own_audio;
  jarray<float, 2>& own_scratch;
  jarray<float, 1> const& smooth_bpm;
  jarray<float, 5> const& all_global_cv;
  jarray<float, 6> const& all_global_audio;
  jarray<void*, 2> const& all_global_context;
  jarray<float, 2> const& own_midi_automation;
  jarray<float, 4> const& all_midi_automation;
  jarray<int, 1> const& own_midi_active_selection;
  jarray<int, 3> const& all_midi_active_selection;
  jarray<float, 3> const& own_accurate_automation;
  jarray<float, 5> const& all_accurate_automation;
  jarray<plain_value, 2> const& own_block_automation;
  jarray<plain_value, 4> const& all_block_automation;
};

// single module process call
struct plugin_block final {
  int start_frame;
  int end_frame;
  int module_slot;
  float sample_rate;
  plugin_block_state state;

  plugin_output_block* out;
  plugin_voice_block* voice;
  shared_block const& host;
  plugin_topo const& plugin;
  module_topo const& module;

  void* module_context(int mod, int slot) const;
  jarray<float, 3> const& module_cv(int mod, int slot) const;
  jarray<float, 4> const& module_audio(int mod, int slot) const;

  void set_out_param(int param, int slot, double raw) const;
  template <domain_type DomainType>
  float normalized_to_raw_fast(int module_, int param_, float normalized) const;
};

inline void*
plugin_block::module_context(int mod, int slot) const
{
  if (plugin.modules[mod].dsp.stage == module_stage::voice)
    return voice->all_context[mod][slot];
  else
    return state.all_global_context[mod][slot];
}

inline jarray<float, 3> const& 
plugin_block::module_cv(int mod, int slot) const
{
  if(plugin.modules[mod].dsp.stage == module_stage::voice)
    return voice->all_cv[mod][slot];
  else
    return state.all_global_cv[mod][slot];
}

inline jarray<float, 4> const& 
plugin_block::module_audio(int mod, int slot) const
{
  if (plugin.modules[mod].dsp.stage == module_stage::voice)
    return voice->all_audio[mod][slot];
  else
    return state.all_global_audio[mod][slot];
}

inline void 
plugin_block::set_out_param(int param, int slot, double raw) const
{
  assert(module.params[param].dsp.direction == param_direction::output);
  out->params[param][slot] = module.params[param].domain.raw_to_plain(raw);
}

template <domain_type DomainType> inline float
plugin_block::normalized_to_raw_fast(int module_, int param_, float normalized) const
{
  check_unipolar(normalized);
  auto const& param_topo = plugin.modules[module_].params[param_];
  return param_topo.domain.normalized_to_raw_fast<DomainType>(normalized_value(normalized));
}

}