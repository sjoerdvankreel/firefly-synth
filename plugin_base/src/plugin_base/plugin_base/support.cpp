#include <plugin_base/support.hpp>

#include <cmath>

namespace plugin_base {

std::vector<std::string>
note_names() 
{ return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

static param_topo
param_base(
  std::string const& id, std::string const& name, int slot_count, int section, std::string const& default_,
  param_dir dir, param_edit edit, param_label label, param_rate rate)
{
  param_topo result = {};
  result.id = id;
  result.dir = dir;
  result.edit = edit;
  result.name = name;
  result.rate = rate;
  result.label = label;
  result.section = section;
  result.default_ = default_;
  result.slot_count = slot_count;
  return result;
}

module_topo
make_module(
  std::string const& id, std::string const& name,
  int slot_count, module_scope scope, module_output output)
{
  module_topo result = {};
  result.id = id;
  result.name = name;
  result.scope = scope;
  result.output = output;
  result.slot_count = slot_count;
  return result;
}

param_topo
param_toggle(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_label label, bool default_)
{
  param_topo result(param_base(id, name, slot_count, section, default_? "On": "Off", dir, param_edit::toggle, label, param_rate::block));
  result.min = 0;
  result.max = 1;
  result.type = param_type::step;
  return result;
}

param_topo
param_steps(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  int min, int max, int default_)
{
  param_topo result(param_base(id, name, slot_count, section, std::to_string(default_), dir, edit, label, param_rate::block));
  result.min = min;
  result.max = max;
  result.type = param_type::step;
  return result;
}

param_topo
param_items(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  std::vector<item_topo>&& items, std::string const& default_)
{
  param_topo result(param_base(id, name, slot_count, section, default_, dir, edit, label, param_rate::block));
  result.items = std::move(items);
  result.min = 0;
  result.max = result.items.size() - 1;
  result.type = param_type::item;
  return result;
}

param_topo
param_names(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  std::vector<std::string> const& names, std::string const& default_)
{
  param_topo result(param_base(id, name, slot_count, section, default_, dir, edit, label, param_rate::block));
  result.min = 0;
  result.max = names.size() - 1;
  result.names = names;
  result.type = param_type::name;
  return result;
}

param_topo
param_pct(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate, bool unit,
  double min, double max, double default_)
{
  param_topo result(param_base(id, name, slot_count, section, std::to_string(default_ * 100), dir, edit, label, rate));
  result.min = min;
  result.max = max;
  result.unit = unit? "%": "";
  result.type = param_type::linear;
  result.display = unit ? param_display::pct : param_display::pct_no_unit;
  return result;
}

param_topo
param_linear(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate,
  double min, double max, double default_, std::string const& unit)
{
  param_topo result(param_base(id, name, slot_count, section, std::to_string(default_), dir, edit, label, rate));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.type = param_type::linear;
  return result;
}

param_topo
param_log(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate,
  double min, double max, double default_, double midpoint, std::string const& unit)
{
  param_topo result(param_base(id, name, slot_count, section, std::to_string(default_), dir, edit, label, rate));
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.type = param_type::log;
  result.exp = std::log((midpoint - min) / (max - min)) / std::log(0.5);
  return result;
}

}