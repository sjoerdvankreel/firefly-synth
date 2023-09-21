#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/block_common.hpp>

#include <cassert>

namespace plugin_base {

struct plugin_topo;

// single output module process call
struct out_process_block final {
  float* const* host_audio;
  jarray<plain_value, 2>& params;
  jarray<float, 2> const& mixdown;
};

// single per-voice module process call
struct voice_process_block final {
  jarray<float, 2>& result;
  voice_state const& state;
  jarray<float, 3> const& cv_in;
  jarray<float, 4> const& audio_in;
};

// single module process call
struct process_block final {
  float sample_rate;
  out_process_block* out;
  common_block const& host;
  plugin_topo const& plugin;
  module_topo const& module;
  voice_process_block* voice;
  jarray<float, 1>& cv_out;
  jarray<float, 2>& audio_out;
  jarray<float, 3> const& global_cv_in;
  jarray<float, 4> const& global_audio_in;
  jarray<float, 3> const& accurate_automation;
  jarray<plain_value, 2> const& block_automation;

  void set_out_param(int param, int slot, double raw);
};

inline void 
process_block::set_out_param(int param, int slot, double raw)
{
  assert(module.params[param].dir == param_dir::output);
  out->params[param][slot] = module.params[param].raw_to_plain(raw);
}

}