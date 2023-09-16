#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class plugin_listener
{
public:
  virtual void plugin_changed(plain_value plain) = 0;
};

class ui_listener
{
public:
  virtual void ui_end_changes(int index) = 0;
  virtual void ui_begin_changes(int index) = 0;
  virtual void ui_changing(int index, plain_value plain) = 0;
};

class plugin_gui:
public juce::Component
{
  plugin_desc const* const _desc;
  jarray<plain_value, 4>* const _ui_state;
  std::vector<ui_listener*> _ui_listeners = {};
  std::vector<std::vector<plugin_listener*>> _plugin_listeners = {};
  std::vector<std::unique_ptr<juce::Component>> _children = {}; // must be destructed first

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  plugin_gui(plugin_desc const* desc, jarray<plain_value, 4>* ui_state);

  void resized() override;
  void paint(juce::Graphics& g) override 
  { g.fillAll(juce::Colours::black); }

  void ui_end_changes(int index);
  void ui_begin_changes(int index);
  void ui_changed(int index, plain_value plain);
  void ui_changing(int index, plain_value plain);
  void plugin_changed(int index, plain_value plain);

  void remove_ui_listener(ui_listener* listener);
  void remove_plugin_listener(int index, plugin_listener* listener);
  void add_ui_listener(ui_listener* listener) { _ui_listeners.push_back(listener); }
  void add_plugin_listener(int index, plugin_listener* listener) { _plugin_listeners[index].push_back(listener); }
};

}