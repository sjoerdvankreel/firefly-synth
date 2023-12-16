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
    iter->second->extra_state_changed();
}

void
extra_state::remove_listener(std::string const& key)
{
  auto iter = _listeners.find(key);
  assert(iter != _listeners.end());
  _listeners.erase(iter);
}

void 
extra_state::add_listener(std::string const& key, extra_state_listener* listener)
{
  auto iter = _listeners.find(key);
  assert(iter == _listeners.end());
  _listeners[key] = listener;
}

void 
extra_state::set_num(std::string const& key, int val)
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
extra_state::get_num(std::string const& key, int default_) const
{
  assert(_keyset.find(key) != _keyset.end());
  auto iter = _values.find(key);
  if(iter == _values.end()) return default_;
  return iter->second.isInt()? iter->second.operator int() : default_;
}

std::string 
extra_state::get_text(std::string const& key, std::string const& default_) const
{
  assert(_keyset.find(key) != _keyset.end());
  auto iter = _values.find(key);
  if (iter == _values.end()) return default_;
  return iter->second.isString()? iter->second.operator juce::String().toStdString(): default_;
}

}