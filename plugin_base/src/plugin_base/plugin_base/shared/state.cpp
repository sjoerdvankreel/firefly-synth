#include <plugin_base/desc/dims.hpp>
#include <plugin_base/shared/state.hpp>

namespace plugin_base {

plugin_state::
plugin_state(plugin_desc const* desc, bool notify):
_desc(desc), _notify(notify)
{
  plugin_dims dims(*_desc->plugin, desc->plugin->audio_polyphony);
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

std::vector<std::string>
plugin_state::undo_stack()
{
  if (_undo_position == 0) return {};
  assert(0 < _undo_position && _undo_position <= _undo_entries.size());
  std::vector<std::string> result;
  for(int i = _undo_position - 1; i >= 0; i--)
    result.push_back(_undo_entries[i]->name);
  return result;
}

std::vector<std::string>
plugin_state::redo_stack()
{
  if(_undo_position == _undo_entries.size()) return {};
  assert(0 <= _undo_position && _undo_position < _undo_entries.size());
  std::vector<std::string> result;
  for(int i = _undo_position; i < _undo_entries.size(); i++)
    result.push_back(_undo_entries[i]->name);
  return result;
}

void 
plugin_state::undo(int index)
{
  assert(_undo_entries.size() > 0);
  assert(0 < _undo_position - index && _undo_position - index <= _undo_entries.size());
  _undo_position -= index + 1;
  copy_from(_undo_entries[_undo_position]->state_before);
}

void 
plugin_state::redo(int index)
{
  assert(_undo_entries.size() > 0);
  assert(0 <= _undo_position + index && _undo_position + index < _undo_entries.size());
  copy_from(_undo_entries[_undo_position + index]->state_after);
  _undo_position += index + 1;
}

void
plugin_state::discard_undo_region()
{
  assert(_undo_region > 0);
  _undo_region = 0;
  _undo_position = 0;
  _undo_entries.clear();
}

void
plugin_state::begin_undo_region()
{
  if(_undo_region == 0) _undo_state_before = jarray<plain_value, 4>(_state);
  _undo_region++;
  assert(_undo_region > 0);
}

void 
plugin_state::end_undo_region(std::string const& name)
{
  assert(_undo_region > 0);
  _undo_region--;
  if(_undo_region != 0) return;
  auto entry = std::make_shared<undo_entry>();
  entry->name = name;
  entry->state_after = jarray<plain_value, 4>(_state);
  entry->state_before = jarray<plain_value, 4>(_undo_state_before);
  _undo_entries.push_back(entry);
  if(_undo_entries.size() > max_undo_size) 
    _undo_entries.erase(_undo_entries.begin());
  _undo_position = _undo_entries.size();
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
plugin_state::copy_from(jarray<plain_value, 4> const& other)
{
  for (int m = 0; m < desc().plugin->modules.size(); m++)
  {
    auto const& module = desc().plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for (int pi = 0; pi < param.info.slot_count; pi++)
          set_plain_at(m, mi, p, pi, other[m][mi][p][pi]);
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