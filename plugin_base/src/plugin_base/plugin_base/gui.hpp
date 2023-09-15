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
  virtual void value_changed(plain_value plain) = 0;
};

class ui_listener
{
public:
  virtual void end_changes(int index) = 0;
  virtual void begin_changes(int index) = 0;
  virtual void changing(int index, plain_value plain) = 0;
};

class plugin_gui:
public juce::Component
{
  plugin_desc const* const _desc;
  std::vector<ui_listener*> _ui_listeners = {};
  std::vector<std::vector<plugin_listener*>> _plugin_listeners = {};
  std::vector<std::unique_ptr<juce::Component>> _children = {}; // must be destructed first

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  plugin_gui(plugin_desc const* desc, jarray<plain_value, 4> const& initial);
  
  void resized() override;
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
    
  void end_changes(int index);
  void begin_changes(int index);
  void changing(int index, plain_value plain);

  // Calls begin-changing-end. Should be the standard behaviour for anything but sliders.
  void ui_param_immediate_changed(int index, plain_value plain);

  void plugin_param_changed(int index, plain_value plain);
  void add_ui_listener(ui_listener* listener);
  void remove_ui_listener(ui_listener* listener);
  void add_plugin_listener(int index, plugin_listener* listener);
  void remove_plugin_listener(int index, plugin_listener* listener);
};

}