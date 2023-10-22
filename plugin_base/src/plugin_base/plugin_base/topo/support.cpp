#include <plugin_base/topo/support.hpp>
#include <cmath>

namespace plugin_base {

topo_tag
make_topo_tag(std::string const& id, std::string const& name)
{
  topo_tag result = {};
  result.id = id;
  result.name = name;
  return result;
}

topo_info
make_topo_info(std::string const& id, std::string const& name, int index, int slot_count)
{
  topo_info result = {};
  result.tag.id = id;
  result.tag.name = name;
  result.index = index;
  result.slot_count = slot_count;
  return result;
}

gui_label
make_label(gui_label_contents contents, gui_label_align align, gui_label_justify justify)
{
  gui_label result = {};
  result.align = align;
  result.justify = justify;
  result.contents = contents;
  return result;
}

param_section
make_param_section(int index, topo_tag const& tag, param_section_gui const& gui)
{
  param_section result = {};
  result.index = index;
  result.tag = topo_tag(tag);
  result.gui = param_section_gui(gui);
  return result;
}

param_section_gui
make_param_section_gui(gui_position const& position, gui_dimension const& dimension)
{
  param_section_gui result = {};
  result.position = position;
  result.dimension = dimension;
  return result;
}

module_section_gui
make_module_section_gui(gui_position const& position, gui_dimension const& dimension)
{
  module_section_gui result = {};
  result.position = position;
  result.dimension = dimension;
  return result; 
}

module_topo
make_module(topo_info const& info, module_dsp const& dsp, module_topo_gui const& gui)
{
  module_topo result = {};
  result.dsp = module_dsp(dsp);
  result.info = topo_info(info);
  result.gui = module_topo_gui(gui);
  return result;
}

module_dsp
make_module_dsp(module_stage stage, module_output output, int output_count, int scratch_count)
{
  module_dsp result = {};
  result.stage = stage;
  result.output = output;
  result.output_count = output_count;
  result.scratch_count = scratch_count;
  return result;
}

module_topo_gui
make_module_gui(int section, gui_position const& position, gui_layout layout, gui_dimension const& dimension)
{
  module_topo_gui result = {};
  result.layout = layout;
  result.position = position;
  result.dimension = dimension;
  return result;
}

param_domain
make_domain_dependent(std::vector<param_domain> const& dependents)
{
  param_domain result = {};
  auto selector = [](auto const& d) { return d.max; };
  auto domain_limits = vector_map(dependents, selector);
  result.min = 0;
  result.default_ = std::to_string(0);
  result.type = domain_type::dependent;
  result.max = *std::max_element(domain_limits.begin(), domain_limits.end());
  return result;
}

param_domain
make_domain_toggle(bool default_)
{
  param_domain result = {};
  result.min = 0;
  result.max = 1;
  result.type = domain_type::toggle;
  result.default_ = default_ ? "On" : "Off";  
  return result;
}

param_domain
make_domain_step(int min, int max, int default_, int display_offset)
{
  param_domain result = {};
  result.min = min;
  result.max = max;
  result.type = domain_type::step;
  result.display_offset = display_offset;
  result.default_ = std::to_string(default_);
  return result;
}

param_domain
make_domain_item(std::vector<list_item> const& items, std::string const& default_)
{
  param_domain result = {};
  result.min = 0;
  result.max = items.size() - 1;
  result.type = domain_type::item;
  result.items = std::vector(items);
  result.default_ = default_.size() ? default_ : result.items[0].name;
  return result;
}

param_domain
make_domain_name(std::vector<std::string> const& names, std::string const& default_)
{
  param_domain result = {};
  result.min = 0;
  result.names = names;
  result.max = names.size() - 1;
  result.type = domain_type::name;
  result.default_ = default_.size() ? default_ : result.names[0];
  return result;
}

param_domain
make_domain_percentage(double min, double max, double default_, int precision, bool unit)
{
  param_domain result = {};
  result.min = min;
  result.max = max;
  result.precision = precision;
  result.unit = unit ? "%" : "";
  result.type = domain_type::linear;
  result.display = domain_display::percentage;
  result.default_ = std::to_string(default_ * 100);
  return result;
}

param_domain
make_domain_linear(double min, double max, double default_, int precision, std::string const& unit)
{
  param_domain result = {};  
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.precision = precision;
  result.type = domain_type::linear;
  result.default_ = std::to_string(default_);
  return result;
}

param_domain
make_domain_log(double min, double max, double default_, double midpoint, int precision, std::string const& unit)
{
  param_domain result = {};
  result.min = min;
  result.max = max;
  result.unit = unit;
  result.precision = precision;
  result.type = domain_type::log;
  result.default_ = std::to_string(default_);
  result.exp = std::log((midpoint - min) / (max - min)) / std::log(0.5);
  return result;
}

param_dsp
make_param_dsp(param_direction direction, param_rate rate, param_automate automate)
{
  param_dsp result = {};
  result.rate = rate;
  result.automate = automate;
  result.direction = direction;
  return result;
}

param_topo_gui
make_param_gui(int section, gui_edit_type edit_type, gui_layout layout, gui_position position, gui_label label)
{
  param_topo_gui result = {};
  result.layout = layout;
  result.section = section;
  result.position = position;
  result.edit_type = edit_type;
  result.label = gui_label(label);
  return result;
}

param_topo
make_param(topo_info const& info, param_dsp const& dsp, param_domain const& domain, param_topo_gui const& gui)
{
  param_topo result = {};
  result.dsp = param_dsp(dsp);
  result.info = topo_info(info);
  result.gui = param_topo_gui(gui);
  result.domain = param_domain(domain);
  return result;
}

}