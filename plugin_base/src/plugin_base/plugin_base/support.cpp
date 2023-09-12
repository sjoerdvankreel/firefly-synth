#include <plugin_base/support.hpp>
#include <cmath>

namespace plugin_base {

std::vector<std::string>
note_names() 
{ return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

static param_topo
input_param(
  std::string const& id, std::string const& name, int group, std::string const& default_, 
  param_display display, param_text text, param_rate rate)
{
  param_topo result = {};
  result.id = id;
  result.name = name;
  result.rate = rate;
  result.text = text;
  result.group = group;
  result.display = display;
  result.default_text = default_;
  result.direction = param_direction::input;
  return result;
}

module_group_topo
make_module_group(
  std::string const& id, std::string const& name,
  int module_count, module_scope scope, module_output output)
{
  module_group_topo result = {};
  result.id = id;
  result.name = name;
  result.scope = scope;
  result.output = output;
  result.module_count = module_count;
  return result;
}

param_topo
param_toggle(
  std::string const& id, std::string const& name, int group,
  param_text text, bool default_)
{
  param_topo result(input_param(id, name, group, default_? "On": "Off", param_display::toggle, text, param_rate::block));
  result.min = 0;
  result.max = 1;
  result.type = param_type::step;
  return result;
}

param_topo
param_steps(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  int min, int max, int default_)
{
  param_topo result(input_param(id, name, group, std::to_string(default_), display, text, param_rate::block));
  result.min = min;
  result.max = max;
  result.type = param_type::step;
  return result;
}

param_topo
param_items(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  items_topo_factory items_factory, std::string const& default_)
{
  param_topo result(input_param(id, name, group, default_, display, text, param_rate::block));
  result.items = items_factory();
  result.min = 0;
  result.max = result.items.size() - 1;
  result.type = param_type::item;
  return result;
}

param_topo
param_names(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  std::vector<std::string> const& names, std::string const& default_)
{
  param_topo result(input_param(id, name, group, default_, display, text, param_rate::block));
  result.min = 0;
  result.max = names.size() - 1;
  result.names = names;
  result.type = param_type::name;
  return result;
}

param_topo
param_percentage(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate, bool unit,
  double min, double max, double default_)
{
  param_topo result(input_param(id, name, group, std::to_string(default_ * 100), display, text, rate));
  result.min = min;
  result.max = max;
  result.unit = unit? "%": "";
  result.type = param_type::linear;
  result.percentage = unit ? param_percentage::on : param_percentage::no_unit;
  return result;
}

param_topo
param_linear(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate,
  double min, double max, double default_, std::string const& unit)
{
  param_topo result(input_param(id, name, group, std::to_string(default_), display, text, rate));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.type = param_type::linear;
  return result;
}

param_topo
param_log(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate,
  double min, double max, double default_, double midpoint, std::string const& unit)
{
  param_topo result(input_param(id, name, group, std::to_string(default_), display, text, rate));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.type = param_type::log;
  result.exp = std::log((midpoint - min) / (max - min)) / std::log(0.5);
  return result;
}

}