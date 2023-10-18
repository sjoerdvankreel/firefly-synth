#include <plugin_base/desc/dims.hpp>
#include <plugin_base/shared/state.hpp>

namespace plugin_base {

plugin_state::
plugin_state(plugin_desc const* desc, bool notify):
_desc(desc), _notify(notify)
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
          set_plain_at(m, mi, p, pi, module.params[p].domain.default_plain());
  }
}

void
plugin_state::state_changed(int index, plain_value plain) const
{
  assert(_notify);
  for(int i = 0; i < _any_listeners.size(); i++)
    _any_listeners[i]->any_state_changed(index, plain);
  auto iter = _listeners.find(index);
  if (iter == _listeners.end()) return;
  for (int i = 0; i < iter->second.size(); i++)
    iter->second[i]->state_changed(index, plain);
}

void 
plugin_state::add_any_listener(any_state_listener* listener) const
{
  assert(_notify);
  _any_listeners.push_back(listener);
}

void
plugin_state::add_listener(int index, state_listener* listener) const
{
  assert(_notify);
  _listeners[index].push_back(listener);
}

void 
plugin_state::remove_any_listener(any_state_listener* listener) const
{
  assert(_notify);
  auto iter = std::find(_any_listeners.begin(), _any_listeners.end(), listener);
  assert(iter != _any_listeners.end());
  _any_listeners.erase(iter);
}

void
plugin_state::remove_listener(int index, state_listener* listener) const
{
  assert(_notify);
  auto map_iter = _listeners.find(index);
  assert(map_iter != _listeners.end());
  auto vector_iter = std::find(map_iter->second.begin(), map_iter->second.end(), listener);
  assert(vector_iter != map_iter->second.end());
  map_iter->second.erase(vector_iter);
}

plain_value
plugin_state::clamp_dependent_at_index(int index, plain_value value) const
{
  auto domain = dependent_domain_at_index(index);
  int result = std::clamp(value.step(), 0, (int)domain->max);
  return domain->raw_to_plain(result);
}

param_domain const*
plugin_state::dependent_domain_at_index(int index) const
{
  auto const& param = *desc().param_at_index(index).param;
  if(param.dependency_indices.size() == 0) return &param.domain;
  int dependency_values[max_param_dependencies_count];
  for(int d = 0; d < param.dependency_indices.size(); d++)
    dependency_values[d] = get_plain_at_index(param.dependency_indices[d]).step();
  return param.dependent_selector(dependency_values);
}

bool 
plugin_state::text_to_normalized_at_index(
  bool io, int index, std::string const& textual, normalized_value& normalized) const
{
  plain_value plain;
  if(!text_to_plain_at_index(io, index, textual, plain)) return false;
  normalized = desc().plain_to_normalized_at_index(index, plain);
  return true;
}

std::string 
plugin_state::plain_to_text_at_index(bool io, int index, plain_value plain) const
{
  auto clamped = clamp_dependent_at_index(index, plain);
  return dependent_domain_at_index(index)->plain_to_text(io, clamped);
}

void
plugin_state::set_plain_at(int m, int mi, int p, int pi, plain_value value)
{
  // get in sync first
  _state[m][mi][p][pi] = value;
  int index = desc().mappings.topo_to_index[m][mi][p][pi];

  auto const& dependents = desc().param_dependents[index];
  for (auto iter = dependents.begin(); iter != dependents.end(); iter++)
  {
    auto const& map = desc().mappings.params[*iter];
    auto dependent_value = get_plain_at_index(*iter);
    auto clamped = clamp_dependent_at_index(*iter, dependent_value);
    set_plain_at(map.module_topo, map.module_slot, map.param_topo, map.param_slot, clamped);
  }
  
  // notify later, even notify if state did not change because of
  // possible multiple bindings and/or changed value-to-text behaviour
  if (_notify) state_changed(index, value);
}

}