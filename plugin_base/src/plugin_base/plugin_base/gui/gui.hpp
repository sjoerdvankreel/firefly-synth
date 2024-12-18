#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/extra_state.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <set>
#include <vector>

namespace plugin_base {

class plugin_gui;
class tab_component;
class grid_component;

std::vector<list_item>
gui_visuals_items();

enum class gui_hover_type { param, module, custom };
enum gui_visuals_mode { gui_visuals_mode_off, gui_visuals_mode_params, gui_visuals_mode_full };

// globally saved (across instances)
inline std::string const user_state_scale_key = "scale";
inline std::string const user_state_theme_key = "theme";

// for serialization
std::set<std::string> gui_extra_state_keyset(plugin_topo const& topo);
std::string module_section_tab_key(plugin_topo const& topo, int section_index);

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

// triggered on selected tab changed
class gui_tab_selection_listener
{
public:
  virtual ~gui_tab_selection_listener() {}
  virtual void module_tab_changed(int module, int slot) {};
  virtual void section_tab_changed(int module, int slot) {};
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
  gui_undo_listener(plugin_gui* gui) : _gui(gui) {}
  void mouseUp(juce::MouseEvent const& event) override;
};

// triggers clear/copy/swap/etc
class gui_tab_menu_listener:
public juce::MouseListener
{
  int const _slot;
  int const _module;
  plugin_gui* const _gui;  
  lnf* const  _lnf;
  plugin_state* const _state;
  juce::TabBarButton* _button;

public:
  void mouseUp(juce::MouseEvent const& event) override;
  ~gui_tab_menu_listener() { _button->removeMouseListener(this); }
  gui_tab_menu_listener(plugin_gui* gui, plugin_state* state, lnf* lnf, juce::TabBarButton* button, int module, int slot):
  _gui(gui), _lnf(lnf), _state(state), _button(button), _module(module), _slot(slot) { _button->addMouseListener(this, true); }
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

class modulation_output_listener
{
public:
  virtual void
  modulation_outputs_reset() = 0;
  virtual void 
  modulation_outputs_changed(std::vector<modulation_output> const& outputs) = 0;
};

class plugin_gui:
public juce::Component
{
  lnf* module_lnf(int index) { return _module_lnfs[index].get(); }
  lnf* custom_lnf(int index) { return _custom_lnfs[index].get(); }

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_gui);
  ~plugin_gui();
  plugin_gui(
    plugin_state* automation_state, plugin_base::extra_state* extra_state,
    std::vector<plugin_base::modulation_output>* modulation_outputs);

  void load_patch();
  void save_patch();
  void init_patch();
  void clear_patch();
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
  void theme_changed(std::string const& theme_name);
  
  // for anyone who wants to repaint on this stuff
  void modulation_outputs_changed(int force_requery_automation_param);
  
  // to keep track of what the audio engine is doing
  void automation_state_changed(int param_index, normalized_value normalized);

  void param_end_changes(int index);
  void param_begin_changes(int index);
  void param_changed(int index, double raw);
  void param_changing(int index, double raw);
  void param_changed(int index, plain_value plain);
  void param_changing(int index, plain_value plain);

  void param_end_changes(int m, int mi, int p, int pi);
  void param_begin_changes(int m, int mi, int p, int pi);
  void param_changed(int m, int mi, int p, int pi, double raw);
  void param_changing(int m, int mi, int p, int pi, double raw);
  void param_changed(int m, int mi, int p, int pi, plain_value plain);
  void param_changing(int m, int mi, int p, int pi, plain_value plain);

  graph_engine* get_module_graph_engine(module_topo const& module);

  void set_system_dpi_scale(float scale);
  float get_system_dpi_scale() const { return _system_dpi_scale; }

  lnf const* get_lnf() const { return _lnf.get(); }
  void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

  std::vector<int> const& engine_voices_active() const { return _engine_voices_active; }
  std::vector<std::uint32_t> const& engine_voices_activated() const { return _engine_voices_activated; }

  gui_visuals_mode get_visuals_mode() const;
  extra_state* extra_state_() const { return _extra_state; }
  plugin_state* automation_state() const { return _automation_state; }
  plugin_state const& global_modulation_state() const { return _global_modulation_state; }
  plugin_state const& voice_modulation_state(int voice) const { return _voice_modulation_states[voice]; }

  void add_modulation_output_listener(modulation_output_listener* listener);
  void remove_modulation_output_listener(modulation_output_listener* listener);
  
  void remove_param_listener(gui_param_listener* listener);
  void remove_gui_mouse_listener(gui_mouse_listener* listener);
  void remove_tab_selection_listener(gui_tab_selection_listener* listener);
  void add_param_listener(gui_param_listener* listener) { _param_listeners.push_back(listener); }
  void add_gui_mouse_listener(gui_mouse_listener* listener) { _gui_mouse_listeners.push_back(listener); }
  void add_tab_selection_listener(gui_tab_selection_listener* listener) { _tab_selection_listeners.push_back(listener); }
  
private:

  std::unique_ptr<lnf> _lnf = {};
  float _system_dpi_scale = 1.0f;
  double _last_mod_reset_seconds = 0.0;  
  gui_visuals_mode _prev_visual_mode = (gui_visuals_mode)-1;

  // this one mirrors the static values of the parameters
  plugin_state* const _automation_state;

  // these ones mirrors what the audio engine is actually doing on a per-block basis
  // not a pointer since this is owned by us, while _automation_state is mutated from outside
  plugin_state _global_modulation_state;
  std::vector<plugin_state> _voice_modulation_states = {};

  gui_undo_listener _undo_listener;
  int _last_mouse_enter_param = -1;
  int _last_mouse_enter_module = -1;
  int _last_mouse_enter_custom = -1;
  plugin_base::extra_state* const _extra_state;
  std::vector<int> _engine_voices_active = {}; // don't like vector bool
  std::vector<std::uint32_t> _engine_voices_activated = {}; // don't like vector bool
  std::vector<plugin_base::modulation_output>* _modulation_outputs = {};
  std::vector<plugin_base::timed_modulation_output> _timed_modulation_outputs = {};
  std::unique_ptr<juce::TooltipWindow> _tooltip = {};
  std::map<int, std::unique_ptr<lnf>> _module_lnfs = {};
  std::map<int, std::unique_ptr<lnf>> _custom_lnfs = {};
  std::map<int, std::unique_ptr<graph_engine>> _module_graph_engines = {};
  std::vector<gui_param_listener*> _param_listeners = {};
  std::vector<gui_mouse_listener*> _gui_mouse_listeners = {};
  std::vector<gui_tab_selection_listener*> _tab_selection_listeners = {};
  std::vector<modulation_output_listener*> _modulation_output_listeners = {};
  // must be destructed first, will unregister listeners, mind order
  std::vector<std::unique_ptr<juce::Component>> _components = {};
  std::vector<std::unique_ptr<gui_hover_listener>> _hover_listeners = {};
  std::vector<std::unique_ptr<gui_tab_menu_listener>> _tab_menu_listeners = {};

  template <class T, class... U>
  T& make_component(U&&... args);

  void fire_state_loaded();
  Component& make_content();

  void set_extra_state_num(std::string const& id, std::string const& part, double val)
  { _extra_state->set_int(id + "/" + part, val); }
  double get_extra_state_num(std::string const& id, std::string const& part, double default_)
  { return _extra_state->get_int(id + "/" + part, default_); }

  void add_tab_menu_listener(juce::TabBarButton& button, int module, int slot);
  void add_hover_listener(juce::Component& component, gui_hover_type type, int global_index);
  void init_multi_tab_component(tab_component& tab, std::string const& id, int module_index, int section_index);

  Component& make_param_sections(module_desc const& module);
  Component& make_param_editor(module_desc const& module, param_desc const& param);
  Component& make_single_param(module_desc const& module, param_desc const& param);
  Component& make_param_label_edit(module_desc const& module, param_desc const& param);
  Component& make_params(module_desc const& module, param_section const& section, param_desc const* params);
  Component& make_param_label(module_desc const& module, param_desc const& param, gui_label_contents contents);
  Component& make_param_section(module_desc const& module, param_section const& section, bool first_horizontal);
  Component& make_multi_param(module_desc const& module, param_section const& section, param_desc const* params);

  Component& make_modules(module_desc const* slots);
  Component& make_module_section(module_section_gui const& section);
  Component& make_custom_section(custom_section_gui const& section);
  void add_component_tab(juce::TabbedComponent& tc, juce::Component& child, int module, std::string const& title);
  tab_component& make_tab_component(std::string const& id, std::string const& title, int module, 
    bool select_tab_on_drag_hover, module_desc const* drag_module_descriptors);
};

}