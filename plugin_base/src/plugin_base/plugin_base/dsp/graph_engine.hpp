#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/shared.hpp>

#include <functional>

namespace plugin_base {

typedef std::function<void(plugin_block&)> graph_processor;

// graph source should decide on this
// e.g. CV sources don't need audio rate to plot
struct module_graph_params
{
  int bpm;
  int sample_rate;
  int frame_count;
  int midi_key = -1;
  int voice_release_at = -1;
};

// utility dsp engine based on static state only
class module_graph_engine {
  plugin_engine _engine;
  jarray<float, 2> _audio;
  plugin_state const* const _state;
  module_graph_params const _params;

  float* _audio_out[2] = {};
  float const* _audio_in[2] = {};
  host_block* _host_block = {};
  std::unique_ptr<plugin_block> _last_block = {};
  std::unique_ptr<plugin_voice_block> _last_voice_block = {};

public:
  plugin_block const* last_block() const { return _last_block.get(); }
  void process(int module_index, int module_slot, graph_processor processor);

  ~module_graph_engine();
  module_graph_engine(plugin_state const* state, module_graph_params const& params);
};

}
