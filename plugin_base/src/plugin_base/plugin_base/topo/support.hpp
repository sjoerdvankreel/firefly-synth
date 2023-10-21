#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <string>

namespace plugin_base {

gui_label 
make_label(gui_label_contents contents, gui_label_align align, gui_label_justify justify);
inline gui_label 
make_label_none()
{ return make_label(gui_label_contents::none, gui_label_align::left, gui_label_justify::center); }

topo_tag
make_topo_tag(std::string const& id, std::string const& name);
topo_info
make_topo_info(std::string const& id, std::string const& name, int index, int slot_count);

section_topo
make_section(int index, topo_tag const& tag, section_topo_gui const& gui);
section_topo_gui
make_section_gui(gui_position const& position, gui_dimension const& dimension);

module_topo
make_module(topo_info const& info, module_dsp const& dsp, module_topo_gui const& gui);
module_dsp
make_module_dsp(module_stage stage, module_output output, int output_count, int scratch_count);
module_topo_gui
make_module_gui(gui_layout layout, gui_position const& position, gui_dimension const& dimension);

param_dsp
make_param_dsp(param_direction direction, param_rate rate, param_automate automate);
inline param_dsp
make_param_dsp_block(param_automate automate)
{ return make_param_dsp(param_direction::input, param_rate::block, automate); }
inline param_dsp
make_param_dsp_accurate(param_automate automate)
{ return make_param_dsp(param_direction::input, param_rate::accurate, automate); }
inline param_dsp
make_param_dsp_output()
{ return make_param_dsp(param_direction::output, param_rate::block, param_automate::none); }

param_domain
make_domain_toggle(bool default_);
param_domain
make_domain_dependent(std::vector<param_domain> const& dependents);
param_domain
make_domain_step(int min, int max, int default_, int display_offset);
param_domain
make_domain_item(std::vector<list_item> const& items, std::string const& default_);
param_domain
make_domain_name(std::vector<std::string> const& names, std::string const& default_);
param_domain
make_domain_percentage(double min, double max, double default_, int precision, bool unit);
param_domain
make_domain_linear(double min, double max, double default_, int precision, std::string const& unit);
param_domain
make_domain_log(double min, double max, double default_, double midpoint, int precision, std::string const& unit);

param_topo
make_param(topo_info const& info, param_dsp const& dsp, param_domain const& domain, param_topo_gui const& gui);
param_topo_gui
make_param_gui(int section, gui_edit_type edit_type, gui_layout layout, gui_position position, gui_label label);
inline param_topo_gui
make_param_gui_single(int section, gui_edit_type edit_type, gui_position position, gui_label label)
{ return make_param_gui(section, edit_type, gui_layout::single, position, gui_label(label)); }

}
