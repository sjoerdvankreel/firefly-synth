#pragma once

#include <map>
#include <set>
#include <string>

namespace plugin_base {

class gui_state final {
  std::map<std::string, double> _num = {};
  std::map<std::string, std::string> _text = {};
  std::set<std::string> const _num_keys = {};
  std::set<std::string> const _text_keys = {};
public:
  gui_state(
    std::set<std::string> const& num_keys, 
    std::set<std::string> const& text_keys): 
  _num_keys(num_keys), _text_keys(text_keys) {}

  void set_num(std::string const& key, double val);
  void load_num(std::string const& key, double val);
  void set_text(std::string const& key, std::string const& val);
  void load_text(std::string const& key, std::string const& val);

  double get_num(std::string const& key, double default_) const;
  std::string get_text(std::string const& key, std::string const& default_) const;
};

}