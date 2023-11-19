#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/value.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <vector>

namespace plugin_base {

class grid_component;

class plugin_gui:
public juce::Component
{
  lnf* module_lnf(int index) 
  { return _module_lnfs[index].get(); }

public:
  INF_PREVENT_ACCIDENTAL_COPY(plugin_gui);
  plugin_gui(plugin_state* gui_state);
  ~plugin_gui() { setLookAndFeel(nullptr); }

  void load_patch();
  void save_patch();
  void init_patch();
  void clear_patch();

  Component& make_load_button();
  Component& make_save_button();
  Component& make_init_button();
  Component& make_clear_button();

  void reloaded();
  void gui_end_changes(int index);
  void gui_begin_changes(int index);
  void gui_changed(int index, plain_value plain);
  void gui_changing(int index, plain_value plain);

  void resized() override;
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

  void remove_listener(gui_listener* listener);
  plugin_state const* gui_state() const { return _gui_state; }
  void add_listener(gui_listener* listener) { _gui_listeners.push_back(listener); }
  
private:
  lnf _lnf;
  plugin_state* const _gui_state;
  std::vector<gui_listener*> _gui_listeners = {};
  std::map<int, std::unique_ptr<lnf>> _module_lnfs = {};
  std::map<int, std::unique_ptr<lnf>> _custom_lnfs = {};
  // must be destructed first, will unregister listeners
  std::vector<std::unique_ptr<juce::Component>> _components = {};

  void fire_state_loaded();
  Component& make_content();

  template <class T, class... U>
  T& make_component(U&&... args);

  Component& make_param_sections(module_desc const& module);
  Component& make_params(module_desc const& module, param_desc const* params);
  Component& make_multi_param(module_desc const& module, param_desc const* params);
  Component& make_param_editor(module_desc const& module, param_desc const& param);
  Component& make_single_param(module_desc const& module, param_desc const& param);
  Component& make_param_label_edit(module_desc const& module, param_desc const& param);
  Component& make_param_section(module_desc const& module, param_section const& section);
  Component& make_param_label(module_desc const& module, param_desc const& param, gui_label_contents contents);

  Component& make_modules(module_desc const* slots);
  Component& make_module_section(module_section_gui const& section);
  Component& make_custom_section(custom_section_gui const& section);
  juce::TabbedComponent& make_tab_component(std::string const& title, int module);
  void add_component_tab(juce::TabbedComponent& tc, juce::Component& child, int module, std::string const& title);
};

}