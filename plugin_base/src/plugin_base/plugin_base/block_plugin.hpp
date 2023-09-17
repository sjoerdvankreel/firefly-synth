#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_common.hpp>

namespace plugin_base {

// all modules automation
struct plugin_automation final {
  jarray<float, 5> accurate = {};
  jarray<plain_value, 4> block = {};
  INF_DECLARE_MOVE_ONLY(plugin_automation);
};

// all modules cv/audio
struct plugin_module_out final {
  jarray<float, 4> voice_cv = {};
  jarray<float, 3> global_cv = {};
  jarray<float, 5> voice_audio = {};
  jarray<float, 4> global_audio = {};
  INF_DECLARE_MOVE_ONLY(plugin_module_out);
};

// global process call in/out
struct plugin_block final {
  float sample_rate = {};
  common_block const* host = {};
  plugin_module_out module_out = {};
  plugin_automation automation = {};
  jarray<float, 3> voices_audio_out = {};
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// voice level module state
struct module_voice_in final {
  jarray<float, 3> const* cv_ = {};
  jarray<float, 4> const* audio_ = {};
  jarray<float, 3> const& cv() const { return *cv_; }
  jarray<float, 4> const& audio() const { return *audio_; }
};

// single module automation and state
struct module_in final {
  module_voice_in const* voice = {};
  jarray<float, 3> const* accurate_ = {};
  jarray<plain_value, 2> const* block_ = {};
  jarray<float, 3> const& accurate() const { return *accurate_; }
  jarray<plain_value, 2> const& block() const { return *block_; }
  INF_DECLARE_MOVE_ONLY(module_in);
};

// single module cv/audio/out params
struct module_out final {
  jarray<float, 1>* cv_ = {};
  jarray<float, 2>* audio_ = {};
  jarray<float, 2>* voice_ = {};
  jarray<plain_value, 2>* params_ = {};
  jarray<float, 1>& cv() const { return *cv_; }
  jarray<float, 2>& audio() const { return *audio_; }
  jarray<float, 2>& voice() const { return *voice_; }
  jarray<plain_value, 2>& params() const { return *params_; }
  INF_DECLARE_MOVE_ONLY(module_out);
};

// single module process call in/out
struct module_block final {
  module_in in;
  module_out out;
  INF_DECLARE_MOVE_ONLY(module_block);
};

}