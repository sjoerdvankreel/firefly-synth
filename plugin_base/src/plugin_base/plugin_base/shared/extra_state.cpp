#include <plugin_base/gui/gui.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/extra_state.hpp>
#include <cassert>

using namespace juce;

namespace plugin_base {

void
extra_state::clear()
{
  _values.clear();
  for(auto const& k: _keyset)
    fire_changed(k);
}

void 
extra_state::fire_changed(std::string const& key)
{
  auto iter = _listeners.find(key);
  if(iter != _listeners.end())
    for(int i = 0; i < iter->second.size(); i++)
      iter->second[i]->extra_state_changed();
}

void
extra_state::remove_listener(std::string const& key, extra_state_listener* listener)
{
  auto key_iter = _listeners.find(key);
  assert(key_iter != _listeners.end());
  auto listener_iter = std::find(key_iter->second.begin(), key_iter->second.end(), listener);
  assert(listener_iter != key_iter->second.end());
  key_iter->second.erase(listener_iter);
}

void 
extra_state::set_int(std::string const& key, int val)
{
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = val;
  fire_changed(key);
}

void 
extra_state::set_text(std::string const& key, std::string const& val)
{
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = String(val);
  fire_changed(key);
}

void
extra_state::set_normalized(std::string const& key, double val)
{
  assert(0.0 <= val && val <= 1.0);
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = val;
  fire_changed(key);
}

var
extra_state::get_var(std::string const& key) const
{
  assert(_keyset.find(key) != _keyset.end());
  assert(_values.find(key) != _values.end());
  return _values.at(key);
}

void
extra_state::set_var(std::string const& key, var const& val)
{
  if(_keyset.find(key) == _keyset.end()) return;
  _values[key] = val;
  fire_changed(key);
}

int 
extra_state::get_int(std::string const& key, int default_) const
{
  assert(_keyset.find(key) != _keyset.end());
  auto iter = _values.find(key);
  if(iter == _values.end()) return default_;
  return iter->second.isInt()? iter->second.operator int() : default_;
}

double
extra_state::get_normalized(std::string const& key, double default_) const
{
  assert(0.0 <= default_ && default_ <= 1.0);
  assert(_keyset.find(key) != _keyset.end());
  auto iter = _values.find(key);
  if (iter == _values.end()) return default_;
  return std::clamp(iter->second.isDouble() ? iter->second.operator double() : default_, 0.0, 1.0);
}

std::string 
extra_state::get_text(std::string const& key, std::string const& default_) const
{
  assert(_keyset.find(key) != _keyset.end());
  auto iter = _values.find(key);
  if (iter == _values.end()) return default_;
  return iter->second.isString()? iter->second.operator juce::String().toStdString(): default_;
}

void
init_instance_from_extra_state(
  extra_state const& extra, 
  plugin_state& gui_state, gui_param_listener* listener)
{
  // per-instance params are single-module single-slot
  auto const& topo = *gui_state.desc().plugin;
  for (int m = 0; m < topo.modules.size(); m++)
    if (topo.modules[m].info.slot_count == 1)
      for (int p = 0; p < topo.modules[m].params.size(); p++)
        if (topo.modules[m].params[p].info.per_instance_key.size())
        {
          auto const& param = topo.modules[m].params[p];
          auto norm_val = extra.get_normalized(param.info.per_instance_key, param.domain.default_normalized(0, 0).value());
          int param_index = gui_state.desc().param_mappings.topo_to_index[m][0][p][0];
          auto plain_val = gui_state.desc().normalized_to_plain_at_index(param_index, normalized_value(norm_val));
          gui_state.set_plain_at_index(param_index, plain_val);
          listener->gui_param_changed(param_index, plain_val);
        }
}

}