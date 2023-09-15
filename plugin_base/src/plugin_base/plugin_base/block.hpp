#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  short key;
  short channel;
};

// keyboard event
struct note_event final {
  int frame;
  note_id id;
  float velocity;
  enum class type { on, off, cut } type;
};

// once per block automation
struct block_event final {
  int param;
  normalized_value normalized;
};

// sample accurate automation
struct accurate_event final {
  int frame;
  int param;
  normalized_value normalized;
};

// shared host/plug
struct common_block final {
  float bpm;
  int frame_count;
  float* const* audio_out;
  float const* const* audio_in;
  std::int64_t stream_time;
  std::vector<note_event> notes;
  INF_DECLARE_MOVE_ONLY(common_block);
};

// shared host/plug
struct host_events final {
  std::vector<block_event> out;
  std::vector<block_event> block;
  std::vector<accurate_event> accurate;
  INF_DECLARE_MOVE_ONLY(host_events);
};

// shared block, automation events
struct host_block final {
  host_events events;
  common_block* common;
  INF_DECLARE_MOVE_ONLY(host_block);
};

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