#pragma once

#include <plugin_base/dsp/value.hpp>
#include <plugin_base/topo/domain.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <string>
#include <algorithm>

namespace plugin_base {

struct module_topo;

enum class param_rate { accurate, block };
enum class param_direction { input, output };

// parameter dsp
struct param_dsp final {
  param_rate rate;
  param_direction direction;

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

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo_gui);
};

// parameter in module
struct param_topo final {
  param_dsp dsp;
  topo_info info;
  param_topo_gui gui;
  param_domain domain;
  int dependent_index;
  std::vector<param_domain> dependents;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo);
  void validate(module_topo const& module, int index) const;
  plain_value clamp_dependent(int dependent_index_, plain_value plain) const;
};

inline plain_value 
param_topo::clamp_dependent(int dependent_index_, plain_value plain) const
{
  auto const& dependent_domain = dependents[dependent_index_];
  return dependent_domain.raw_to_plain(std::clamp(plain.step(), 0, (int)dependent_domain.max));
}

}
