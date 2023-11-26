#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/shared.hpp>

#include <functional>

namespace plugin_base {

typedef std::function<graph_data(plugin_block&)> graph_renderer;

// graph source should decide on this
// e.g. CV sources don't need audio rate to plot
struct module_graph_params
{
  int bpm;
  int sample_rate;
  int frame_count;
  int module_slot;
  int module_index;
  int voice_release_at = -1;
};

// utility dsp engine based on static state only
class module_graph_engine {
  plugin_engine _engine;
  jarray<float, 2> _audio;
  plugin_state const* const _state;
  module_graph_params const _params;

public:
  ~module_graph_engine() { _engine.deactivate(); }
  graph_data render(graph_renderer renderer);
  module_graph_engine(plugin_state const* state, module_graph_params const& params);
};

}
