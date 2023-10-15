#include <plugin_base/desc/dims.hpp>
#include <plugin_base/shared/state.hpp>

namespace plugin_base {

plugin_state::
plugin_state(plugin_desc const* desc):
_desc(desc)
{
  plugin_dims dims(*_desc->plugin);
  _state.resize(dims.module_slot_param_slot);
  init_defaults();
}

void
plugin_state::init_defaults()
{
  for (int m = 0; m < desc().plugin->modules.size(); m++)
  {
    auto const& module = desc().plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for (int pi = 0; pi < module.params[p].info.slot_count; pi++)
          _state[m][mi][p][pi] = module.params[p].domain.default_plain();
  }
}

void
plugin_notifier::plugin_changed(int index, plain_value plain)
{
  auto iter = _listeners.find(index);
  if(iter == _listeners.end()) return;
  for(int i = 0; i < iter->second.size(); i++)
    iter->second[i]->plugin_changed(index, plain);
}

void
plugin_notifier::remove_listener(int index, plugin_listener* listener)
{
  auto map_iter = _listeners.find(index);
  assert(map_iter != _listeners.end());
  auto vector_iter = std::find(map_iter->second.begin(), map_iter->second.end(), listener);
  assert(vector_iter != map_iter->second.end());
  map_iter->second.erase(vector_iter);
}

}