#include <plugin_base/desc/dims.hpp>
#include <plugin_base/shared/state.hpp>

namespace plugin_base {

plugin_state::
plugin_state(plugin_desc const* desc, bool notify):
_desc(desc), _notify(notify)
{
  plugin_dims dims(*_desc->plugin);
  _state.resize(dims.module_slot_param_slot);
  init(state_init_type::default_);
}

void 
plugin_state::set_text_at(int m, int mi, int p, int pi, std::string const& value)
{
  plain_value plain;
  PB_ASSERT_EXEC(desc().param_topo_at(m, p).domain.text_to_plain(false, value, plain));
  set_plain_at(m, mi, p, pi, plain);
}

void
plugin_state::state_changed(int index, plain_value plain) const
{
  assert(_notify);
  auto iter = _listeners.find(index);
  if (iter == _listeners.end()) return;
  for (int i = 0; i < iter->second.size(); i++)
    iter->second[i]->state_changed(index, plain);
  for(int i = 0; i < _any_listeners.size(); i++)
    _any_listeners[i]->any_state_changed(index, plain);
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

bool 
plugin_state::text_to_normalized_at_index(
  bool io, int index, std::string const& textual, normalized_value& normalized) const
{
  plain_value plain;
  if(!text_to_plain_at_index(io, index, textual, plain)) return false;
  normalized = desc().plain_to_normalized_at_index(index, plain);
  return true;
}

void
plugin_state::set_plain_at(int m, int mi, int p, int pi, plain_value value)
{
  if(!_notify)
  {
    _state[m][mi][p][pi] = value;
    return;
  }
  bool changed;
  if(desc().plugin->modules[m].params[p].domain.is_real())
    changed = _state[m][mi][p][pi].real() != value.real();
  else
    changed = _state[m][mi][p][pi].step() != value.step();
  _state[m][mi][p][pi] = value;
  if (_notify && changed) 
    state_changed(desc().param_mappings.topo_to_index[m][mi][p][pi], value);
}

void
plugin_state::move_module_to(int index, int source_slot, int target_slot)
{
  copy_module_to(index, source_slot, target_slot);
  clear_module(index, source_slot);
}

void
plugin_state::clear_module_all(int index)
{
  auto const& topo = desc().plugin->modules[index];
  for (int mi = 0; mi < topo.info.slot_count; mi++)
    clear_module(index, mi);
}

void
plugin_state::insert_module_after(int index, int slot)
{
  auto const& topo = desc().plugin->modules[index];
  assert(0 <= slot && slot < topo.info.slot_count - 1);
  clear_module(index, topo.info.slot_count - 1);
  for(int i = topo.info.slot_count - 1; i > slot + 1; i--)
    move_module_to(index, i - 1, i);
}

void
plugin_state::insert_module_before(int index, int slot)
{
  auto const& topo = desc().plugin->modules[index];
  (void)topo;
  assert(0 < slot && slot < topo.info.slot_count);
  clear_module(index, 0);
  for (int i = 0; i < slot - 1; i++)
    move_module_to(index, i + 1, i);
}

void 
plugin_state::clear_module(int index, int slot)
{
  auto const& topo = desc().plugin->modules[index];
  (void)topo;
  for(int p = 0; p < topo.params.size(); p++)
    for(int pi = 0; pi < topo.params[p].info.slot_count; pi++)
      set_plain_at(index, slot, p, pi, topo.params[p].domain.default_plain(slot, pi));
}

void 
plugin_state::copy_module_to(int index, int source_slot, int target_slot)
{
  auto const& topo = desc().plugin->modules[index];
  for (int p = 0; p < topo.params.size(); p++)
    for (int pi = 0; pi < topo.params[p].info.slot_count; pi++)
    {
      auto slot_selector = topo.params[p].gui.bindings.enabled.slot_selector;
      if(slot_selector == nullptr || slot_selector(target_slot))
        set_plain_at(index, target_slot, p, pi, get_plain_at(index, source_slot, p, pi));
    }
}

void 
plugin_state::swap_module_with(int index, int source_slot, int target_slot)
{
  auto const& topo = desc().plugin->modules[index];
  for (int p = 0; p < topo.params.size(); p++)
    for (int pi = 0; pi < topo.params[p].info.slot_count; pi++)
    {
      auto slot_selector = topo.params[p].gui.bindings.enabled.slot_selector;
      plain_value target = get_plain_at(index, target_slot, p, pi);
      if (slot_selector == nullptr || slot_selector(target_slot))
        set_plain_at(index, target_slot, p, pi, get_plain_at(index, source_slot, p, pi));
      if (slot_selector == nullptr || slot_selector(source_slot))
        set_plain_at(index, source_slot, p, pi, target);
    }
}

void 
plugin_state::copy_from(plugin_state const* state)
{
  for (int m = 0; m < desc().plugin->modules.size(); m++)
  {
    auto const& module = desc().plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for (int pi = 0; pi < param.info.slot_count; pi++)
          set_plain_at(m, mi, p, pi, state->get_plain_at(m, mi, p, pi));
      }
  }
}

void
plugin_state::init(state_init_type init_type)
{
  for (int m = 0; m < desc().plugin->modules.size(); m++)
  {
    auto const& module = desc().plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for (int pi = 0; pi < module.params[p].info.slot_count; pi++)
          set_plain_at(m, mi, p, pi, module.params[p].domain.default_plain(mi, pi));
  }
  for(int m = 0; m < desc().plugin->modules.size(); m++)
    if(init_type == state_init_type::minimal && desc().plugin->modules[m].minimal_initializer)
      desc().plugin->modules[m].minimal_initializer(*this);
    else if (init_type == state_init_type::default_ && desc().plugin->modules[m].default_initializer)
      desc().plugin->modules[m].default_initializer(*this);
}

}