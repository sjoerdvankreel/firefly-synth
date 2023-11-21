#include <plugin_base/shared/extra_state.hpp>
#include <cassert>

using namespace juce;

namespace plugin_base {

void
extra_state::remove_listener(extra_state_listener* listener)
{
  auto iter = std::find(_listeners.begin(), _listeners.end(), listener);
  assert(iter != _listeners.end());
  _listeners.erase(iter);
}

void 
extra_state::set_num(std::string const& key, int val)
{
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = val;
}

void
extra_state::set_var(std::string const& key, var const& val)
{
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = val;
}

void 
extra_state::set_text(std::string const& key, std::string const& val)
{
  assert(_keyset.find(key) != _keyset.end());
  _values[key] = String(val);
}

var
extra_state::get_var(std::string const& key) const
{
  assert(_keyset.find(key) != _keyset.end());
  assert(_values.find(key) != _values.end());
  return _values.at(key);
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