#pragma once

#include <plugin_base/topo/domain.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <string>
#include <algorithm>

namespace plugin_base {

struct param_topo;
struct module_topo;

enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_automate { none, automate, modulate };
enum class param_layout { single, horizontal, vertical };

// parameter dsp
struct param_dsp final {
  param_rate rate;
  param_automate automate;
  param_direction direction;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_dsp);
};

// parameter ui
struct param_topo_gui final {
  int section;
  gui_label label;
  param_layout layout;
  gui_position position;
  gui_bindings bindings;
  gui_edit_type edit_type;
  std::shared_ptr<gui_submenu> submenu;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo_gui);
  bool is_list() const;
  void validate(module_topo const& module, param_topo const& param) const;
};

// parameter in module
struct param_topo final {
  param_dsp dsp;
  topo_info info;
  param_topo_gui gui;
  param_domain domain;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo);
  void validate(module_topo const& module, int index) const;
};

inline bool
param_topo_gui::is_list() const
{ return edit_type == gui_edit_type::list || edit_type == gui_edit_type::autofit_list; }

}
