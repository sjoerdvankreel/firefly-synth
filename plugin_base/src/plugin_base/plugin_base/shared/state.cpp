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
  auto iter = _listeners.find(index);
  if (iter == _listeners.end()) return;
  for (int i = 0; i < iter->second.size(); i++)
    iter->second[i]->state_changed(index, plain);
}

void 
plugin_state::add_listener(int index, state_listener* listener) const
{
  assert(_notify);
  _listeners[index].push_back(listener);
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
  int dependency_index = desc().dependency_index(index);
  if (dependency_index == -1) return desc().param_at_index(index).param->domain.plain_to_text(io, plain);
  int dependency_value = get_plain_at_index(dependency_index).step();
  auto clamped = desc().param_at_index(index).param->clamp_dependent(dependency_value, plain);
  return desc().param_at_index(index).param->dependent_domains[dependency_value].plain_to_text(io, clamped);
}

bool 
plugin_state::text_to_plain_at_index(
  bool io, int index, std::string const& textual, plain_value& plain) const
{
  int dependency_index = desc().dependency_index(index);
  if (dependency_index == -1) return desc().param_at_index(index).param->domain.text_to_plain(io, textual, plain);
  int dependency_value = get_plain_at_index(dependency_index).step();
  return desc().param_at_index(index).param->dependent_domains[dependency_value].text_to_plain(io, textual, plain);
}

void
plugin_state::set_plain_at(int m, int mi, int p, int pi, plain_value value)
{
  int index = desc().mappings.topo_to_index[m][mi][p][pi];
  auto const& param = desc().param_at_index(index).param;
  if (param->domain.is_real() && _state[m][mi][p][pi].real() == value.real()) return;
  if (!param->domain.is_real() && _state[m][mi][p][pi].step() == value.step()) return;
  _state[m][mi][p][pi] = value;
  if (_notify) state_changed(index, value);
  for (int d = 0; d < desc().param_dependents[index].size(); d++)
  {
    int dependent_index = desc().param_dependents[index][d];
    auto dependent_value = get_plain_at_index(dependent_index);
    auto clamped = desc().param_at_index(dependent_index).param->clamp_dependent(value.step(), dependent_value);
    set_plain_at_index(dependent_index, clamped);
  }
}

}