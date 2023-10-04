#include <plugin_base/support.hpp>

#include <cmath>

namespace plugin_base {

std::vector<std::string>
note_names() 
{ return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

static param_topo
param_base(
  std::string const& id, std::string const& name, int index, int slot_count, int section, std::string const& default_,
  param_dir dir, param_rate rate, param_format format, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result = {};
  result.id = id;
  result.dir = dir;
  result.edit = edit;
  result.name = name;
  result.rate = rate;
  result.index = index;
  result.format = format;
  result.layout = layout;
  result.section = section;
  result.position = position;
  result.slot_count = slot_count;
  result.domain.default_ = default_;
  result.label_align = label_align;
  result.label_justify = label_justify;
  result.label_contents = label_contents;
  return result;
}

int
index_of_item_tag(std::vector<item_topo> const& items, int tag)
{
  for(int i = 0; i < items.size(); i++)
    if(items[i].tag == tag)
      return i;
  assert(false);
  return -1;
}

section_topo
make_section(
  std::string const& name, int index,
  gui_position const& position, gui_dimension const& dimension)
{
  section_topo result = {};
  result.name = name;
  result.index = index;
  result.position = position;
  result.dimension = dimension;
  return result;
}

module_topo
make_module(
  std::string const& id, std::string const& name, int index, int slot_count, 
  module_stage stage, module_output output, int output_count,
  gui_layout layout, gui_position const& position, gui_dimension const& dimension)
{
  module_topo result = {};
  result.id = id;
  result.name = name;
  result.index = index;
  result.stage = stage;
  result.output = output;
  result.layout = layout;
  result.position = position;
  result.dimension = dimension;
  result.slot_count = slot_count;
  result.output_count = output_count;
  return result;
}

param_topo
param_toggle(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  bool default_,
  param_dir dir,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, default_? "On": "Off", 
    dir, param_rate::block, param_format::plain, param_edit::toggle, 
    label_contents, label_align, label_justify, layout, position));
  result.domain.min = 0;
  result.domain.max = 1;
  result.type = param_type::step;
  return result;
}

param_topo
param_steps(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  int min, int max, int default_,
  param_dir dir, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, std::to_string(default_), 
    dir, param_rate::block, param_format::plain, edit,
    label_contents, label_align, label_justify, layout, position));
  result.domain.min = min;
  result.domain.max = max;
  result.type = param_type::step;
  return result;
}

param_topo
param_items(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  std::vector<item_topo>&& items, std::string const& default_,
  param_dir dir, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, default_.size()? default_: items[0].name,
    dir, param_rate::block, param_format::plain, edit,
    label_contents, label_align, label_justify, layout, position));
  result.items = std::move(items);
  result.domain.min = 0;
  result.domain.max = result.items.size() - 1;
  result.type = param_type::item;
  return result;
}

param_topo
param_names(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  std::vector<std::string> const& names, std::string const& default_,
  param_dir dir, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, default_.size()? default_: names[0],
    dir, param_rate::block, param_format::plain, edit,
    label_contents, label_align, label_justify, layout, position));
  result.domain.min = 0;
  result.domain.max = names.size() - 1;
  result.names = names;
  result.type = param_type::name;
  return result;
}

param_topo
param_pct(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, int precision,
  param_dir dir, param_rate rate, param_format format, bool unit, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, std::to_string(default_ * 100), 
    dir, rate, format, edit, 
    label_contents, label_align, label_justify, layout, position));
  result.domain.min = min;
  result.domain.max = max;
  result.domain.unit = unit? "%": "";
  result.precision = precision;
  result.type = param_type::linear;
  result.display = param_display::pct;
  return result;
}

param_topo
param_linear(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, int precision, std::string const& unit,
  param_dir dir, param_rate rate, param_format format, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, std::to_string(default_), 
    dir, rate, format, edit,
    label_contents, label_align, label_justify, layout, position));
  result.domain.min = min;
  result.domain.max = max;
  result.domain.unit = unit;
  result.precision = precision;
  result.type = param_type::linear;
  return result;
}

param_topo
param_log(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, double midpoint, int precision, std::string const& unit,
  param_dir dir, param_rate rate, param_format format, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position)
{
  param_topo result(param_base(
    id, name, index, slot_count, section, std::to_string(default_), 
    dir, rate, format, edit,
    label_contents, label_align, label_justify, layout, position));
  result.precision = precision;
  result.type = param_type::log;
  result.domain.min = min;
  result.domain.max = max;
  result.domain.unit = unit;
  result.domain.exp = std::log((midpoint - min) / (max - min)) / std::log(0.5);
  return result;
}

}