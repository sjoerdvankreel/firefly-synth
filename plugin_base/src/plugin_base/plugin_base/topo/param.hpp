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

typedef std::function<int(int const* values)>
dependent_domain_selector;
inline int const max_param_dependencies_count = 4;

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
  gui_layout layout;
  gui_position position;
  gui_bindings bindings;
  gui_edit_type edit_type;
  std::vector<gui_submenu> submenus;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo_gui);
  bool is_list() const;
  void validate(module_topo const& module, param_topo const& param) const;
};

// dependent parameter bindings
struct dependent_param final {
  std::vector<int> dependencies = {};
  std::vector<param_domain> domains = {};
  dependent_domain_selector selector = {};

  void bind(
    std::vector<int> const& dependencies_, 
    std::vector<param_domain> const& domains_, 
    dependent_domain_selector selector_);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(dependent_param);
};

// parameter in module
struct param_topo final {
  param_dsp dsp;
  topo_info info;
  param_topo_gui gui;
  param_domain domain;
  dependent_param dependent;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo);
  void validate(module_topo const& module, int index) const;
};

inline bool
param_topo_gui::is_list() const
{ return edit_type == gui_edit_type::list || edit_type == gui_edit_type::autofit_list; }

}
