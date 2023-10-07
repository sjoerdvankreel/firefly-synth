#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/topo/domain.hpp>
#include <plugin_base/topo/shared.hpp>

#include <vector>
#include <string>

namespace plugin_base {

struct module_topo;

enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_format { plain, normalized };

// parameter dsp
struct param_dsp final {
  param_rate rate;
  param_format format;
  param_direction direction;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_dsp);
};

// parameter ui
struct param_topo_gui final {
  gui_layout layout;
  gui_position position;
  gui_bindings bindings;
  gui_edit_type edit_type;
  gui_label_align label_align;
  gui_label_justify label_justify;
  gui_label_contents label_contents;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_topo_gui);
};

// parameter in module
struct param_topo final {
  int section;
  param_dsp dsp;
  topo_info info;
  param_topo_gui gui;
  param_domain domain;

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_topo);
  void validate(module_topo const& module) const;
};

}
