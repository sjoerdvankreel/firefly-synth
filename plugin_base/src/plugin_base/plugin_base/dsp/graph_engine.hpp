#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/shared.hpp>

#include <map>
#include <functional>

namespace plugin_base {

typedef std::function<void(plugin_block&)> graph_processor;

// graph source should decide on this
// e.g. CV sources don't need audio rate to plot
struct graph_engine_params
{
  int bpm;
  int max_frame_count;
  int midi_key = -1;
};

// utility dsp engine based on static state only
class graph_engine {
  plugin_engine _engine;
  int _sample_rate = -1;
  int _voice_release_at = -1;

  plugin_desc const* _desc = {};
  jarray<float, 2> _audio_in = {};
  jarray<float, 2> _audio_out = {};
  graph_engine_params const _params;

  host_block* _host_block = {};
  float* _audio_out_ptrs[2] = {};
  float const* _audio_in_ptrs[2] = {};
  std::unique_ptr<plugin_block> _last_block = {};
  std::unique_ptr<plugin_voice_block> _last_voice_block = {};
  std::map<int, std::map<int, std::unique_ptr<module_engine>>> _activated = {};

public:
  ~graph_engine() { _engine.deactivate(); }
  graph_engine(plugin_desc const* desc, graph_engine_params const& params);

  void process_end();
  void process_begin(plugin_state const* state, int sample_rate, int frame_count, int voice_release_at);
  plugin_block const* process_default(int module_index, int module_slot);
  plugin_block const* process(int module_index, int module_slot, graph_processor processor);
};

}
