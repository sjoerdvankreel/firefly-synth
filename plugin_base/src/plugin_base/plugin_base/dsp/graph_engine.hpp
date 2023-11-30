#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/shared.hpp>

#include <functional>

namespace plugin_base {

typedef std::function<void(plugin_block&)> graph_processor;

// graph source should decide on this
// e.g. CV sources don't need audio rate to plot
struct graph_engine_params
{
  int bpm;
  int sample_rate;
  int frame_count;
  int midi_key = -1;
  int voice_release_at = -1;
};

// utility dsp engine based on static state only
class graph_engine {
  plugin_engine _engine;
  jarray<float, 2> _audio_in = {};
  jarray<float, 2> _audio_out = {};
  plugin_state const* const _state;
  graph_engine_params const _params;

  host_block* _host_block = {};
  float* _audio_out_ptrs[2] = {};
  float const* _audio_in_ptrs[2] = {};
  std::unique_ptr<plugin_block> _last_block = {};
  std::unique_ptr<plugin_voice_block> _last_voice_block = {};

public:
  ~graph_engine();
  graph_engine(plugin_state const* state, graph_engine_params const& params);

  plugin_block const* process_default(int module_index, int module_slot);
  plugin_block const* process(int module_index, int module_slot, graph_processor processor);
};

}
