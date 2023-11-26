#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/shared.hpp>

#include <functional>

namespace plugin_base {

typedef std::function<graph_data(plugin_block&)> graph_renderer;

// graph source should decide on this
// e.g. CV sources don't need audio rate to plot
struct graph_engine_params
{
  int bpm;
  int sample_rate;
  int frame_count;
};

// utility dsp engine based on static state only
class graph_engine {
  plugin_engine _engine;
  plugin_state const* const _state;
  graph_engine_params const _params;

public:
  ~graph_engine() { _engine.deactivate(); }
  graph_data render(graph_renderer renderer);
  graph_engine(plugin_desc const* desc, plugin_state const* state, graph_engine_params const& params);
};

}
