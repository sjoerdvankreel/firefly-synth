#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/extra_state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <set>
#include <vector>

namespace plugin_base {

class plugin_gui;
class tab_component;
class grid_component;

enum class gui_hover_type { param, module, custom };

// for serialization
inline std::string const factory_preset_key = "factory_preset";
std::set<std::string> gui_extra_state_keyset(plugin_topo const& topo);

// helper for vertical sizing
struct gui_vertical_section_size { bool header; float size_relative; };
std::vector<int> 
gui_vertical_distribution(int total_height, int font_height, 
  std::vector<gui_vertical_section_size> const& section_sizes);

// gui mouse events for anyone who needs it
class gui_mouse_listener
{
public:
  virtual ~gui_mouse_listener() {}
  virtual void param_mouse_exit(int param) {};
  virtual void param_mouse_enter(int param) {};
  virtual void module_mouse_exit(int module) {};
  virtual void module_mouse_enter(int module) {};
  virtual void custom_mouse_exit(int section) {};
  virtual void custom_mouse_enter(int section) {};
};

// tracking gui parameter changes
// host binding is expected to update actual gui/controller state
class gui_param_listener
{
public:
  virtual ~gui_param_listener() {}
  void gui_param_changed(int index, plain_value plain);

  virtual void gui_param_end_changes(int index) = 0;
  virtual void gui_param_begin_changes(int index) = 0;
  virtual void gui_param_changing(int index, plain_value plain) = 0;
};

// triggers undo/redo
class gui_undo_listener:
public juce::MouseListener
{
  plugin_gui* const _gui;
public:
  void mouseUp(juce::MouseEvent const& event);
  gui_undo_listener(plugin_gui* gui): _gui(gui) {}
};

// triggers clear/copy/swap/etc
class gui_tab_menu_listener:
public juce::MouseListener
{
  int const _slot;
  int const _module;
  plugin_state* const _state;
  juce::TabBarButton* _button;

public:
  void mouseUp(juce::MouseEvent const& event);
  ~gui_tab_menu_listener() { _button->removeMouseListener(this); }
  gui_tab_menu_listener(plugin_state* state, juce::TabBarButton* button, int module, int slot):
  _state(state), _button(button), _module(module), _slot(slot) { _button->addMouseListener(this, true); }
};

// triggers gui_mouse_listener
class gui_hover_listener:
public juce::MouseListener
{
  plugin_gui* const _gui;
  int const _global_index;
  gui_hover_type const _type;
  juce::Component* const _component;
public:
  void mouseExit(juce::MouseEvent const&) override;
  void mouseEnter(juce::MouseEvent const&) override;
  ~gui_hover_listener() { _component->removeMouseListener(this); }
  gui_hover_listener(plugin_gui* gui, juce::Component* component, gui_hover_type type, int global_index):
  _gui(gui), _global_index(global_index), _type(type), _component(component) { _component->addMouseListener(this, true); }
};

class plugin_gui:
public juce::Component
{
  lnf* module_lnf(int index) { return _module_lnfs[index].get(); }
  lnf* custom_lnf(int index) { return _custom_lnfs[index].get(); }

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_gui);
  ~plugin_gui();
  plugin_gui(plugin_state* gui_state, plugin_base::extra_state* extra_state);

  void save_patch();
  void init_patch();
  void clear_patch();

  void load_patch();
  void load_patch(std::string const& path, bool preset);

  Component& make_load_button();
  Component& make_save_button();
  Component& make_init_button();
  Component& make_clear_button();

  void param_mouse_exit(int param);
  void param_mouse_enter(int param);
  void module_mouse_exit(int module);
  void module_mouse_enter(int module);
  void custom_mouse_exit(int section);
  void custom_mouse_enter(int section);

  void reloaded();
  void resized() override;
  void param_end_changes(int index);
  void param_begin_changes(int index);
  void param_changed(int index, plain_value plain);
  void param_changing(int index, plain_value plain);

  plugin_state* gui_state() const { return _gui_state; }
  extra_state* extra_state() const { return _extra_state; }
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
  
  void remove_param_listener(gui_param_listener* listener);
  void remove_gui_mouse_listener(gui_mouse_listener* listener);
  void add_param_listener(gui_param_listener* listener) { _param_listeners.push_back(listener); }
  void add_gui_mouse_listener(gui_mouse_listener* listener) { _gui_mouse_listeners.push_back(listener); }
  
private:
  lnf _lnf;
  juce::TooltipWindow _tooltip;
  plugin_state* const _gui_state;
  gui_undo_listener _undo_listener;
  plugin_base::extra_state* const _extra_state;
  std::map<int, std::unique_ptr<lnf>> _module_lnfs = {};
  std::map<int, std::unique_ptr<lnf>> _custom_lnfs = {};
  std::vector<gui_param_listener*> _param_listeners = {};
  std::vector<gui_mouse_listener*> _gui_mouse_listeners = {};
  // must be destructed first, will unregister listeners, mind order
  std::vector<std::unique_ptr<juce::Component>> _components = {};
  std::vector<std::unique_ptr<gui_hover_listener>> _hover_listeners = {};
  std::vector<std::unique_ptr<gui_tab_menu_listener>> _tab_menu_listeners = {};

  template <class T, class... U>
  T& make_component(U&&... args);

  void fire_state_loaded();
  Component& make_content();
  void init_multi_tab_component(tab_component& tab, std::string const& id);
  
  void add_tab_menu_listener(juce::TabBarButton& button, int module, int slot);
  void add_hover_listener(juce::Component& component, gui_hover_type type, int global_index);

  void set_extra_state_num(std::string const& id, std::string const& part, double val)
  { _extra_state->set_num(id + "/" + part, val); }
  double get_extra_state_num(std::string const& id, std::string const& part, double default_)
  { return _extra_state->get_num(id + "/" + part, default_); }

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
  tab_component& make_tab_component(std::string const& id, std::string const& title, int module);
  void add_component_tab(juce::TabbedComponent& tc, juce::Component& child, int module, std::string const& title);
};

}