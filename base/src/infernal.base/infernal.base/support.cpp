#include <infernal.base/support.hpp>

namespace infernal::base {

std::vector<std::string>
note_names() 
{ return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

static param_topo
input_block_param(
  std::string const& id, std::string const& name, int group, std::string const& default_)
{
  param_topo result = {};
  result.id = id;
  result.name = name;
  result.group = group;
  result.default_text = default_;
  result.rate = param_rate::block;
  result.direction = param_direction::input;
  return result;
}

param_topo
param_toggle(
  std::string const& id, std::string const& name, int group,
  bool default_)
{
  param_topo result(input_block_param(id, name, group, default_? "On": "Off"));
  result.min = 0;
  result.max = 1;
  result.type = param_type::step;
  result.display = param_display::toggle;
  return result;
}

param_topo
param_steps(
  std::string const& id, std::string const& name, int group,
  param_display display, int min, int max, int default_)
{
  param_topo result(input_block_param(id, name, group, std::to_string(default_)));
  result.min = min;
  result.max = max;
  result.display = display;
  result.type = param_type::step;
  return result;
}

param_topo
param_percentage(
  std::string const& id, std::string const& name, int group,
  param_display display, double min, double max, double default_)
{
  param_topo result(input_block_param(id, name, group, std::to_string(default_)));
  result.min = min;
  result.max = max;
  result.unit = "%";
  result.display = display;
  result.percentage = true;
  result.type = param_type::linear;
  return result;
}

param_topo
param_linear(
  std::string const& id, std::string const& name, int group,
  param_display display, double min, double max, double default_, std::string const& unit)
{
  param_topo result(input_block_param(id, name, group, std::to_string(default_)));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.display = display;
  result.type = param_type::linear;
  return result;
}

param_topo
param_names(
  std::string const& id, std::string const& name, int group,
  param_display display, std::vector<std::string> const& names, std::string const& default_)
{
  param_topo result(input_block_param(id, name, group, default_));
  result.min = 0;
  result.max = names.size() - 1;
  result.display = display;
  result.type = param_type::name;
  return result;
}

param_topo
param_items(
  std::string const& id, std::string const& name, int group,
  param_display display, std::vector<item_topo> const& items, std::string const& default_)
{
  param_topo result(input_block_param(id, name, group, default_));
  result.min = 0;
  result.max = items.size() - 1;
  result.display = display;
  result.type = param_type::item;
  return result;
}

param_topo
param_log(
  std::string const& id, std::string const& name, int group,
  param_display display, double min, double max, double default_, double midpoint, std::string const& unit)
{
  param_topo result(input_block_param(id, name, group, std::to_string(default_)));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.display = display;
  result.type = param_type::log;
  return result;
}