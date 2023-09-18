#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_common.hpp>

namespace plugin_base {

struct plugin_topo;

// single output module process call
struct out_process_block final {
  float* const* host_audio;
  jarray<plain_value, 2>& params;
  jarray<float, 2> const& mixdown;
  INF_DECLARE_MOVE_ONLY(out_process_block);
};

// single per-voice module process call
struct voice_process_block final {
  jarray<float, 2>& result;
  voice_state const& state;
  jarray<float, 3> const& cv_in;
  jarray<float, 4> const& audio_in;
  INF_DECLARE_MOVE_ONLY(voice_process_block);
};

// single module process call
struct process_block final {
  float sample_rate;
  out_process_block* out;
  common_block const& host;
  plugin_desc const& plugin;
  module_desc const& module;
  voice_process_block* voice;
  jarray<float, 1>& cv_out;
  jarray<float, 2>& audio_out;
  jarray<float, 3> const& global_cv_in;
  jarray<float, 4> const& global_audio_in;
  jarray<float, 3> const& accurate_automation;
  jarray<plain_value, 2> const& block_automation;

  INF_DECLARE_MOVE_ONLY(process_block);
  void set_out_param(int param, int slot, double raw)
  { out->params[param][slot] = module.params[param].topo->raw_to_plain(raw); }
};

}