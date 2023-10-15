#pragma once

#include <plugin_base/topo/domain.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/value.hpp>
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
  int dependency_index;
  std::vector<param_domain> dependency_domains;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_topo);
  void validate(module_topo const& module, int index) const;
  plain_value clamp_dependent(int dependency_value, plain_value dependent_value) const;
};

inline plain_value 
param_topo::clamp_dependent(int dependency_value, plain_value dependent_value) const
{
  auto const& dependency_domain = dependency_domains[dependency_value];
  int clamped = std::clamp(dependent_value.step(), 0, (int)dependency_domain.max);
  return dependency_domain.raw_to_plain(clamped);
}

}
