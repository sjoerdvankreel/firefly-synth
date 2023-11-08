#include <plugin_base/topo/support.hpp>

#include <map>
#include <cmath>
#include <numeric>

namespace plugin_base {

static default_selector
simple_default(std::string value)
{ return [value](int, int) { return value; }; }

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
make_module_section_gui_none(int index)
{
  module_section_gui result = {};
  result.index = index;
  result.visible = false;
  return result;
}

module_section_gui
make_module_section_gui(int index, gui_position const& position, gui_dimension const& dimension)
{
  module_section_gui result = {};
  result.index = index;
  result.tabbed = false;
  result.visible = true;
  result.position = position;
  result.dimension = dimension;
  return result; 
}

module_section_gui
make_module_section_gui_tabbed(int index, gui_position const& position, std::string const& header, int tab_width, std::vector<int> const& tab_order)
{
  module_section_gui result = {};
  result.index = index;
  result.tabbed = true;
  result.visible = true;
  result.position = position;
  result.tab_header = header;
  result.tab_width = tab_width;
  result.tab_order = tab_order;
  result.dimension = { 1, 1 };
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
make_module_dsp(module_stage stage, module_output output, int scratch_count, std::vector<topo_info> const& outputs)
{
  module_dsp result = {};
  result.stage = stage;
  result.output = output;
  result.scratch_count = scratch_count;
  result.outputs = vector_explicit_copy(outputs);
  return result;
}

module_topo_gui
make_module_gui_none(int section)
{
  module_topo_gui result = {};
  result.visible = false;
  result.section = section;
  return result;
}

module_topo_gui
make_module_gui(int section, gui_colors const& colors, gui_position const& position, gui_dimension const& dimension)
{
  module_topo_gui result = {};
  result.visible = true;
  result.section = section;
  result.position = position;
  result.dimension = dimension;
  result.colors = gui_colors(colors);
  return result;
}

param_domain
make_domain_toggle(bool default_)
{
  param_domain result = {};
  result.min = 0;
  result.max = 1;
  result.type = domain_type::toggle;
  result.default_selector = simple_default(default_ ? "On" : "Off");  
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
  result.default_selector = simple_default(std::to_string(default_));
  return result;
}

param_domain
make_domain_timesig(std::vector<timesig> const& sigs, timesig const& default_)
{
  param_domain result = {};
  result.min = 0;
  result.timesigs = sigs;
  result.max = sigs.size() - 1;
  result.type = domain_type::timesig;
  result.default_selector = simple_default(default_.to_text());
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
  result.default_selector = simple_default(default_.size() ? default_ : result.items[0].name);
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
  result.default_selector = simple_default(default_.size() ? default_ : result.names[0]);
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
  result.default_selector = simple_default(std::to_string(default_ * 100));
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
  result.default_selector = simple_default(std::to_string(default_));
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
  result.default_selector = simple_default(std::to_string(default_));
  result.exp = std::log((midpoint - min) / (max - min)) / std::log(0.5);
  return result;
}

param_dsp
make_param_dsp(param_direction direction, param_rate rate, param_automate automate)
{
  param_dsp result = {};
  result.rate = rate;
  result.direction = direction;
  result.automate_selector = [automate](int) { return automate; };
  return result;
}

param_topo_gui
make_param_gui(int section, gui_edit_type edit_type, param_layout layout, gui_position position, gui_label label)
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

std::shared_ptr<gui_submenu>
make_timesig_submenu(std::vector<timesig> const& sigs)
{
  std::map<int, std::vector<int>> sigs_by_num;
  for (int i = 0; i < sigs.size(); i++)
    sigs_by_num[sigs[i].num].push_back(i);
  auto result = std::make_shared<gui_submenu>();
  for(auto const& sbn: sigs_by_num)
  {
    auto sig_sub = std::make_shared<gui_submenu>();
    sig_sub->name = std::to_string(sbn.first);
    sig_sub->indices = sbn.second;
    result->children.push_back(sig_sub);
  }
  return result;
}

std::vector<timesig>
make_timesigs(std::vector<int> const& steps, timesig low, timesig high)
{
  assert(low.den > 0);
  assert(high.den > 0);
  assert(steps.size());

  std::vector<timesig> result;
  for (int n = 0; n < steps.size(); n++)
    for (int d = 0; d < steps.size(); d++)
      result.push_back({ steps[n], steps[d] });

  auto filter_low = [low](auto const& s) { return (float)s.num / s.den >= (float)low.num / low.den; };
  result = vector_filter(result, filter_low);
  auto filter_high = [high](auto const& s) { return (float)s.num / s.den <= (float)high.num / high.den; };
  return vector_filter(result, filter_high);
}

}