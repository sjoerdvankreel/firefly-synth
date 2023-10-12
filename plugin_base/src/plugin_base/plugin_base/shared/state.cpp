#include <plugin_base/desc/dims.hpp>
#include <plugin_base/shared/state.hpp>

namespace plugin_base {

plugin_state::
plugin_state(std::unique_ptr<plugin_topo>&& topo):
_desc(std::move(topo))
{
  plugin_dims dims(*_desc.plugin);
  _state.resize(dims.module_slot_param_slot);
  _desc.init_defaults(_state);
}

}