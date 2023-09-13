#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class single_param_plugin_listener
{
public:
  virtual void plugin_value_changed(plain_value plain) = 0;
};

class any_param_ui_listener
{
public:
  virtual void ui_param_end_changes(int param_index) = 0;
  virtual void ui_param_begin_changes(int param_index) = 0;
  virtual void ui_param_changing(int param_index, plain_value plain) = 0;
};

class plugin_gui:
public juce::Component
{
  plugin_desc const _desc;
  std::vector<any_param_ui_listener*> _any_param_ui_listeners = {};
  std::vector<std::vector<single_param_plugin_listener*>> _single_param_plugin_listeners = {};
  // Note - mind reverse order of destruction. 
  // Param_* controls must deregister their event listeners before the respective listener vectors are destroyed.
  std::vector<std::unique_ptr<juce::Component>> _children = {};

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  plugin_gui(plugin_topo_factory factory, jarray4d<plain_value> const& initial);
  
  void resized() override;
  plugin_desc const& desc() const { return _desc; }
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
    
  void ui_param_end_changes(int param_index);
  void ui_param_begin_changes(int param_index);
  void ui_param_changing(int param_index, plain_value plain);

  // Calls begin-changing-end. Should be the standard behaviour for anything but sliders.
  void ui_param_immediate_changed(int param_index, plain_value plain);

  void plugin_param_changed(int param_index, plain_value plain);
  void add_any_param_ui_listener(any_param_ui_listener* listener);
  void remove_any_param_ui_listener(any_param_ui_listener* listener);
  void add_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener);
  void remove_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener);
};

}