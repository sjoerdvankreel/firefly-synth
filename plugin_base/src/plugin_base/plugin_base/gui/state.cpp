#include <plugin_base/gui/state.hpp>
#include <cassert>

namespace plugin_base {

void 
gui_state::set_num(std::string const& key, double val)
{
  assert(_num_keys.find(key) != _num_keys.end());
  _num[key] = val;
}

void 
gui_state::load_num(std::string const& key, double val)
{
  if(_num_keys.find(key) != _num_keys.end())
    _num[key] = val;
}

void 
gui_state::set_text(std::string const& key, std::string const& val)
{
  assert(_text_keys.find(key) != _text_keys.end());
  _text[key] = val;
}

void 
gui_state::load_text(std::string const& key, std::string const& val)
{
  if (_text_keys.find(key) != _text_keys.end())
    _text[key] = val;
}

double 
gui_state::get_num(std::string const& key, double default_) const
{
  assert(_num_keys.find(key) != _num_keys.end());
  auto iter = _num.find(key);
  if(iter == _num.end()) return default_;
  return iter->second;
}

std::string 
gui_state::get_text(std::string const& key, std::string const& default_) const
{
  assert(_text_keys.find(key) != _text_keys.end());
  auto iter = _text.find(key);
  if (iter == _text.end()) return default_;
  return iter->second;
}

}