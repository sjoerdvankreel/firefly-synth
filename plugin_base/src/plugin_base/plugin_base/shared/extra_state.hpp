#pragma once

#include <plugin_base/shared/utility.hpp>
#include <juce_core/juce_core.h>

#include <map>
#include <set>
#include <string>
#include <functional>

namespace plugin_base {

class extra_state_listener {
public:
  virtual ~extra_state_listener() {}
  virtual void extra_state_changed() = 0;
};

// per-instance controller state
class extra_state final {
  std::set<std::string> _keyset = {};
  std::map<std::string, juce::var> _values = {};
  std::map<std::string, extra_state_listener*> _listeners = {};

  void fire_changed(std::string const& key);

public:
  PB_PREVENT_ACCIDENTAL_COPY(extra_state);
  extra_state(std::set<std::string> const& keyset) : _keyset(keyset) {}

  void clear() { _values.clear(); }
  std::set<std::string> const& keyset() const { return _keyset; }
  bool contains_key(std::string const& key) const { return _values.find(key) != _values.end(); }

  void set_num(std::string const& key, int val);
  void set_var(std::string const& key, juce::var const& val);
  void set_text(std::string const& key, std::string const& val);

  void remove_listener(std::string const& key);
  void add_listener(std::string const& key, extra_state_listener* listener);

  juce::var get_var(std::string const& key) const;
  int get_num(std::string const& key, int default_) const;
  std::string get_text(std::string const& key, std::string const& default_) const;
};

}