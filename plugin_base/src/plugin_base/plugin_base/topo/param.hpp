#pragma once

#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/domain.hpp>
#include <plugin_base/topo/midi_source.hpp>

#include <vector>
#include <string>
#include <algorithm>

namespace plugin_base {

struct param_topo;
struct module_topo;
struct plugin_topo;
class plugin_state;

enum class param_direction { input, output };
enum class param_rate { block, voice, accurate };
enum class param_automate { none, midi, automate, modulate };
enum class param_layout { 
  single, horizontal, vertical, 
  single_grid // must be single child in parent container
};

// allows to extend right-click menu on parameters
class param_menu_handler {
protected:
  plugin_state* const _state;
  param_menu_handler(plugin_state* state): _state(state) {}

public:
  virtual ~param_menu_handler() {}
  virtual std::vector<custom_menu> const menus() const = 0;

  // must return the item that action was executed upon
  virtual std::string execute(int menu_id, int action, int module_index, int module_slot, int param_index, int param_slot) = 0;
};

typedef std::function<param_automate(int module_slot)>
automate_selector;
typedef std::function<bool(int that_val, int this_val)>
gui_item_binding_selector;
typedef std::function<std::unique_ptr<param_menu_handler>(plugin_state*)>
param_menu_handler_factory;

// binding to item enabled
struct gui_item_binding final {
  bool auto_bind = false;
  param_topo_mapping param = {};
  gui_item_binding_selector selector = {};
  static inline int const match_param_slot = -1;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_item_binding);

  bool is_param_bound() const { return selector != nullptr; }
  bool is_bound() const { return is_param_bound() || auto_bind; }
  void bind_param(param_topo_mapping param_, gui_item_binding_selector selector_);
};

// parameter ui
struct param_topo_gui final {
  int section;
  bool visible;
  bool tabular;
  gui_label label;
  param_layout layout;
  gui_position position;
  gui_bindings bindings;
  gui_edit_type edit_type;
  // autosize to these if not empty
  std::string label_reference_text; 
  std::string value_reference_text;
  gui_item_binding item_enabled = {};
  std::shared_ptr<gui_submenu> submenu;
  param_menu_handler_factory menu_handler_factory;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo_gui);
  bool is_list() const;
  void validate(plugin_topo const& plugin, module_topo const& module, param_topo const& param) const;
};

// parameter dsp
struct param_dsp final {
  param_rate rate;
  param_direction direction;
  midi_topo_mapping midi_source;
  automate_selector automate_selector_;

  bool is_midi(int module_slot) const;
  bool can_modulate(int module_slot) const;
  bool can_automate(int module_slot) const;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_dsp);
  void validate(plugin_topo const& plugin, module_topo const& module, int module_slot) const;
};

// parameter in module
struct param_topo final {
  param_dsp dsp;
  topo_info info;
  param_topo_gui gui;
  param_domain domain;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo);
  void validate(plugin_topo const& plugin, module_topo const& module, int index) const;
};

inline bool
param_dsp::is_midi(int module_slot) const
{
  auto mode = automate_selector_(module_slot);
  return mode == param_automate::midi;
}

inline bool 
param_dsp::can_modulate(int module_slot) const
{
  auto mode = automate_selector_(module_slot);
  return mode == param_automate::modulate;
}

inline bool
param_dsp::can_automate(int module_slot) const
{
  auto mode = automate_selector_(module_slot);
  return mode == param_automate::automate || mode == param_automate::modulate;
}

inline bool
param_topo_gui::is_list() const
{ return edit_type == gui_edit_type::list || edit_type == gui_edit_type::autofit_list; }

}
