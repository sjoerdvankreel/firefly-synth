#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class single_param_plugin_listener
{
public:
  virtual void 
  plugin_value_changed(param_value value) = 0;
};

class any_param_ui_listener
{
public:
  virtual void 
  ui_value_changed(int param_index, param_value value) = 0;
};

class plugin_gui:
public juce::Component
{
  plugin_desc const _desc;
  std::vector<any_param_ui_listener*> _any_param_ui_listeners;
  std::vector<std::vector<single_param_plugin_listener*>> _single_param_plugin_listeners; 

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  plugin_gui(plugin_topo_factory factory);

  void resized() override;
  plugin_desc const& desc() const { return _desc; }
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

  void ui_param_changed(int param_index, param_value value);
  void plugin_param_changed(int param_index, param_value value);
  void add_any_param_ui_listener(any_param_ui_listener* listener);
  void remove_any_param_ui_listener(any_param_ui_listener* listener);
  void add_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener);
  void remove_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener);
};

}