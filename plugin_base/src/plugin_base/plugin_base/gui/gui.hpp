#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/value.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class grid_component;

class gui_listener
{
public:
  void gui_changed(int index, plain_value plain);
  virtual void gui_end_changes(int index) = 0;
  virtual void gui_begin_changes(int index) = 0;
  virtual void gui_changing(int index, plain_value plain) = 0;
};

class plugin_gui:
public juce::Component
{
public:
  INF_PREVENT_ACCIDENTAL_COPY(plugin_gui);
  plugin_gui(plugin_state* gui_state);

  void gui_end_changes(int index);
  void gui_begin_changes(int index);
  void gui_changed(int index, plain_value plain);
  void gui_changing(int index, plain_value plain);

  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
  void resized() override { getChildComponent(0)->setBounds(getLocalBounds()); }
  void content_scale(float factor) { setTransform(juce::AffineTransform::scale(factor)); }

  void fire_state_loaded();
  void remove_listener(gui_listener* listener);
  plugin_state const* gui_state() const { return _gui_state; }
  void add_listener(gui_listener* listener) { _gui_listeners.push_back(listener); }
  
private:
  plugin_state* const _gui_state;
  std::vector<gui_listener*> _gui_listeners = {};
  // must be destructed first, will unregister listeners
  std::vector<std::unique_ptr<juce::Component>> _components = {};

  Component& make_top_bar();
  Component& make_content();
  Component& make_container();

  template <class T, class... U>
  T& make_component(U&&... args);
  template <class Topo, class Slot, class MakeSingle>
  Component& make_multi_slot(Topo const& topo, Slot const* slots, MakeSingle make_single);

  Component& make_modules(module_desc const* slots);
  Component& make_multi_module(module_desc const* slots);
  Component& make_single_module(module_desc const& slot, bool tabbed);
  Component& make_sections(module_desc const& module);
  Component& make_section(module_desc const& module, section_topo const& section);
  Component& make_params(module_desc const& module, param_desc const* params);
  Component& make_param_label(module_desc const& module, param_desc const& param);
  Component& make_param_editor(module_desc const& module, param_desc const& param);
  Component& make_multi_param(module_desc const& module, param_desc const* params);
  Component& make_single_param(module_desc const& module, param_desc const& param);
  Component& make_param_label_edit(module_desc const& module, param_desc const& param,
      gui_dimension const& dimension, gui_position const& label_position, gui_position const& edit_position);
};

}