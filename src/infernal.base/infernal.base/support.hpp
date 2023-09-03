#pragma once
#include <infernal.base/topo.hpp>
#include <string>

namespace infernal::base {

param_config config_input_toggle();
param_config config_input_step_list();
param_config config_input_step_knob();
param_config config_input_step_hslider();
param_config config_input_step_vslider();
param_config config_input_list_list();
param_config config_input_list_knob();
param_config config_input_list_hslider();
param_config config_input_list_vslider();
param_config config_input_log_block_knob();
param_config config_input_log_block_hslider();
param_config config_input_log_block_vslider();
param_config config_input_log_accurate_knob();
param_config config_input_log_accurate_hslider();
param_config config_input_log_accurate_vslider();
param_config config_input_linear_block_knob();
param_config config_input_linear_block_hslider();
param_config config_input_linear_block_vslider();
param_config config_input_linear_accurate_knob();
param_config config_input_linear_accurate_hslider();
param_config config_input_linear_accurate_vslider();

std::vector<item_topo>
note_name_items();

module_group_topo
make_module_group(
  std::string const& id, std::string const& name, 
  int module_count, module_scope scope, module_output output);
param_topo
make_param_toggle(
  std::string const& id, std::string const& name, std::string const& default_,
  int group, param_config const& config);
param_topo
make_param_pct(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, int group, param_config const& config);
param_topo
make_param_step(
  std::string const& id, std::string const& name, std::string const& default_,
  int min, int max, int group, param_config const& config);
param_topo
make_param_list(
  std::string const& id, std::string const& name, std::string const& default_,
  std::vector<item_topo> const& items, int group, param_config const& config);param_topo
make_param_log(
  std::string const& id, std::string const& name, std::string const& default_, std::string const& unit,
  double min, double max, double midpoint, int group, param_config const& config);

}
