#pragma once

#include <plugin_base/dsp.hpp>
#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/block_common.hpp>

#include <cassert>

namespace plugin_base {

struct plugin_topo;

// single output module process call
struct out_process_block final {
  int voice_count;
  int thread_count;
  double cpu_usage;
  float* const* host_audio;
  jarray<plain_value, 2>& params;
  jarray<float, 2> const& mixdown;
};

// single per-voice module process call
struct voice_process_block final {
  bool finished;
  jarray<float, 2>& result;
  voice_state const& state;
  jarray<float, 4> const& cv_in;
  jarray<float, 5> const& audio_in;
};

// single module process call
struct process_block final {
  int start_frame;
  int end_frame;
  float sample_rate;
  out_process_block* out;
  common_block const& host;
  plugin_topo const& plugin;
  module_topo const& module;
  voice_process_block* voice;
  jarray<float, 2>& cv_out;
  jarray<float, 3>& audio_out;
  jarray<float, 4> const& global_cv_in;
  jarray<float, 5> const& global_audio_in;
  jarray<float, 3> const& accurate_automation;
  jarray<plain_value, 2> const& block_automation;

  void set_out_param(int param, int slot, double raw) const;
  float normalized_to_raw(int module_, int param_, float normalized) const;
};

inline void 
process_block::set_out_param(int param, int slot, double raw) const
{
  assert(module.params[param].dsp.direction == param_direction::output);
  out->params[param][slot] = module.params[param].domain.raw_to_plain(raw);
}

inline float
process_block::normalized_to_raw(int module_, int param_, float normalized) const
{
  check_unipolar(normalized);
  auto const& param_topo = plugin.modules[module_].params[param_];
  return param_topo.domain.normalized_to_raw(normalized_value(normalized));
}

}