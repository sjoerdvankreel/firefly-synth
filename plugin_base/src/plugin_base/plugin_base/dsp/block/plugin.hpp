#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/block/shared.hpp>

#include <Client/libMTSClient.h>

#include <cassert>

namespace plugin_base {

struct plugin_topo;

enum class voice_stage { unused, active, releasing, finishing };

// for monophonic mode
enum class mono_note_stream_event { none, on, off };

// for monophonic mode
struct mono_note_state
{
  int midi_key = -1;
  mono_note_stream_event event_type = mono_note_stream_event::none;
};

// for MTS-ESP tuning
struct note_tuning
{
  bool is_mapped = false;
  float retuned_semis = -1;
  float retuned_frequency = -1;
};

// needs cooperation from the plug
enum engine_tuning_mode {
  engine_tuning_mode_off, // no microtuning
  engine_tuning_mode_on_note_before_mod, // query at voice start, tune before modulation
  engine_tuning_mode_on_note_after_mod, // query at voice start, tune after modulation
  engine_tuning_mode_continuous_before_mod, // query each block, tune before modulation
  engine_tuning_mode_continuous_after_mod // query each block, tune after modulation
};

// needs cooperation from the plug
enum engine_tuning_interpolation { 
  engine_tuning_interpolation_linear, // lerp pitch
  engine_tuning_interpolation_log // lerp log2(frequency)
};

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

  // are we graphing?
  bool graph;

  // MTS-ESP support
  MTSClient* mts_client = {};
  // If disabled, unused (we go with pitch-to-freq without an intermediate table).
  // If per-block, points to a table populated at block start.
  // If per-voice, points to a table populated at voice start.
  std::array<note_tuning, 128>* current_tuning = nullptr;
  engine_tuning_mode current_tuning_mode = (engine_tuning_mode)-1;

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

  // mts-esp support
  template <engine_tuning_mode TuningMode, engine_tuning_interpolation TuningInterpolation>
  float pitch_to_freq_with_tuning(float pitch);

  void set_out_param(int param, int slot, double raw) const;
  
  template <domain_type DomainType>
  float normalized_to_raw_fast(int module_, int param_, float normalized) const;
  template <domain_type DomainType>
  void normalized_to_raw_block(int module_, int param_, jarray<float, 1> const& in, jarray<float, 1>& out) const;
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

template <domain_type DomainType> inline void
plugin_block::normalized_to_raw_block(int module_, int param_, jarray<float, 1> const& in, jarray<float, 1>& out) const
{
  for(int f = start_frame; f < end_frame; f++) check_unipolar(in[f]);
  auto const& param_topo = plugin.modules[module_].params[param_];
  param_topo.domain.normalized_to_raw_block<DomainType>(in, out, start_frame, end_frame);
}

template <engine_tuning_mode TuningMode, engine_tuning_interpolation TuningInterpolation>
inline float
plugin_block::pitch_to_freq_with_tuning(float pitch)
{
  if constexpr (TuningMode == engine_tuning_mode_off)
    return pitch_to_freq_no_tuning(pitch);

  // these 2 cases are already tuned beforehand
  else if constexpr (TuningMode == engine_tuning_mode_on_note_before_mod)
    return pitch_to_freq_no_tuning(pitch);
  else if constexpr (TuningMode == engine_tuning_mode_continuous_before_mod)
    return pitch_to_freq_no_tuning(pitch);

  else if constexpr (TuningMode == engine_tuning_mode_on_note_after_mod || TuningMode == engine_tuning_mode_continuous_after_mod)
  {
    pitch = std::clamp(pitch, 0.0f, 127.0f);
    int pitch_low = (int)std::floor(pitch);
    int pitch_high = (int)std::ceil(pitch);
    float pos = pitch - pitch_low;
    float freq_low = (*current_tuning)[pitch_low].retuned_frequency;
    float freq_high = (*current_tuning)[pitch_high].retuned_frequency;
    if constexpr (TuningInterpolation == engine_tuning_interpolation_linear)
      return (1.0f - pos) * freq_low + pos * freq_high;
    else if constexpr (TuningInterpolation == engine_tuning_interpolation_log)
      return std::pow(2.0f, (1.0f - pos) * std::log2(freq_low) + pos * std::log2(freq_high));
    else
      static_assert(false);
  }

  else static_assert(false);
}

}