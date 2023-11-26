#include <plugin_base/dsp/graph_engine.hpp>

namespace plugin_base {

graph_engine::
graph_engine(plugin_desc const* desc, plugin_state const* state, graph_engine_params const& params):
_engine(desc, nullptr, nullptr), _state(state), _params(params)
{ _engine.activate(_params.sample_rate, _params.frame_count); }

graph_data 
graph_engine::render(graph_renderer renderer)
{
  return {};
}

}