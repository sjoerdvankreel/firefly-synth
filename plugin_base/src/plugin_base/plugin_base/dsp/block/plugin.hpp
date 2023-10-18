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

// for polyphonic synth
struct voice_state final {
  note_id id = {};
  int end_frame = -1;
  int start_frame = -1;
  int release_frame = -1;
  float velocity = 0.0f;
  std::int64_t time = -1;
  voice_stage stage = {};
};

// single output module process call
struct plugin_output_block final {
  int voice_count;
  int thread_count;
  double cpu_usage;
  float* const* host_audio;
  jarray<plain_value, 2>& params;
  jarray<float, 2> const& mixdown;
};

// single per-voice module process call
struct plugin_voice_block final {
  bool finished;
  jarray<float, 2>& result;
  voice_state const& state;
  jarray<float, 4> const& all_cv;
  jarray<float, 5> const& all_audio;
  jarray<void*, 2> const& all_context;
};

// state and automation
struct plugin_block_state final {
  void** own_context = {};
  jarray<float, 2>& own_cv;
  jarray<float, 3>& own_audio;
  jarray<float, 4> const& all_global_cv;
  jarray<float, 5> const& all_global_audio;
  jarray<void*, 2> const& all_global_context;
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

  void set_out_param(int param, int slot, double raw) const;
  float normalized_to_raw(int module_, int param_, float normalized) const;
};

inline void 
plugin_block::set_out_param(int param, int slot, double raw) const
{
  assert(module.params[param].dsp.direction == param_direction::output);
  out->params[param][slot] = module.params[param].domain.raw_to_plain(raw);
}

inline float
plugin_block::normalized_to_raw(int module_, int param_, float normalized) const
{
  check_unipolar(normalized);
  auto const& param_topo = plugin.modules[module_].params[param_];
  return param_topo.domain.normalized_to_raw(normalized_value(normalized));
}

}