#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class grid_component;

void gui_init();
void gui_terminate();

class ui_listener
{
public:
  void ui_changed(int index, plain_value plain);
  virtual void ui_end_changes(int index) = 0;
  virtual void ui_begin_changes(int index) = 0;
  virtual void ui_changing(int index, plain_value plain) = 0;
};

class plugin_listener
{
public:
  virtual void plugin_changed(int index, plain_value plain) = 0;
};

class plugin_gui:
public juce::Component
{
  plugin_desc const* const _desc;
  std::vector<ui_listener*> _ui_listeners = {};
  jarray<plain_value, 4>* const _ui_state = {};
  std::vector<std::vector<plugin_listener*>> _plugin_listeners = {};
  std::vector<std::unique_ptr<juce::Component>> _components = {}; // must be destructed first

  void state_loaded();
  template <class T, class... U>
  T& make_component(U&&... args);
  template <class Topo, class Slot, class MakeSingle>
  Component& make_multi_slot(Topo const& topo, Slot const* slots, MakeSingle make_single);

  Component& make_top_bar();
  Component& make_content();
  Component& make_container();

  Component& make_modules(module_desc const* slots);
  Component& make_multi_module(module_desc const* slots);
  Component& make_single_module(module_desc const& slot, bool tabbed);
  Component& make_sections(module_desc const& module);
  Component& make_section(module_desc const& module, section_topo const& section);
  Component& make_params(module_desc const& module, param_desc const* params);
  Component& make_param_edit(module_desc const& module, param_desc const& param);
  Component& make_param_label(module_desc const& module, param_desc const& param);
  Component& make_multi_param(module_desc const& module, param_desc const* params);
  Component& make_single_param(module_desc const& module, param_desc const& param);
  Component& make_param_label_edit(module_desc const& module, param_desc const& param,
      gui_dimension const& dimension, gui_position const& label_position, gui_position const& edit_position);

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  plugin_gui(plugin_desc const* desc, jarray<plain_value, 4>* ui_state);

  void ui_end_changes(int index);
  void ui_begin_changes(int index);
  void ui_changed(int index, plain_value plain);
  void ui_changing(int index, plain_value plain);
  void plugin_changed(int index, plain_value plain);

  plugin_desc const* desc() const { return _desc; }
  jarray<plain_value, 4> const& ui_state() const { return *_ui_state; }

  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
  void resized() override { getChildComponent(0)->setBounds(getLocalBounds()); }
  void content_scale(float factor) { setTransform(juce::AffineTransform::scale(factor)); }

  void remove_ui_listener(ui_listener* listener);
  void remove_plugin_listener(int index, plugin_listener* listener);
  void add_ui_listener(ui_listener* listener) { _ui_listeners.push_back(listener); }
  void add_plugin_listener(int index, plugin_listener* listener) { _plugin_listeners[index].push_back(listener); }
};

}