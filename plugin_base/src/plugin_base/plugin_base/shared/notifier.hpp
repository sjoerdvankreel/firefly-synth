#pragma once

#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/utility.hpp>

#include <map>
#include <vector>

namespace plugin_base {

class plugin_listener
{
public:
  virtual void plugin_changed(int index, plain_value plain) = 0;
};

class no_notifier final {
public:
  void plugin_changed(int index, plain_value plain) {}
  void add_listener(int index, plugin_listener* listener) {}
  void remove_listener(int index, plugin_listener* listener) {}
};

class plugin_notifier final {
  std::map<int, std::vector<plugin_listener*>> _listeners = {};
public:
  void plugin_changed(int index, plain_value plain);
  void remove_listener(int index, plugin_listener* listener);
  
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_notifier);
  void add_listener(int index, plugin_listener* listener)
  { _listeners[index].push_back(listener); }
};

}