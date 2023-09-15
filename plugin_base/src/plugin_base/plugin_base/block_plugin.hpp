#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

namespace plugin_base {

struct common_block;

// all modules automation
struct plugin_in final {
  jarray<float, 5> accurate = {};
  jarray<plain_value, 4> block = {};
  INF_DECLARE_MOVE_ONLY(plugin_in);
};

// all modules cv/audio
struct plugin_out final {
  jarray<float, 3> cv = {};
  jarray<float, 4> audio = {};
  INF_DECLARE_MOVE_ONLY(plugin_out);
};

// global process call in/out
struct plugin_block final {
  plugin_in in = {};
  plugin_out out = {};
  float sample_rate = {};
  common_block const* host = {};
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// single module automation
struct module_in final {
  jarray<float, 3> const* accurate_ = {};
  jarray<plain_value, 2> const* block_ = {};
  jarray<float, 3> const& accurate() { return *accurate_; }
  jarray<plain_value, 2> const& block() { return *block_; }
  INF_DECLARE_MOVE_ONLY(module_in);
};

// single module cv/audio/out params
struct module_out final {
  jarray<float, 1>* cv_ = {};
  jarray<float, 2>* audio_ = {};
  jarray<plain_value, 2>* params_ = {};
  jarray<float, 1>& cv() const { return *cv_; }
  jarray<float, 2>& audio() const { return *audio_; }
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