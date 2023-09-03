#include <infernal.base/support.hpp>

namespace infernal::base {

static param_topo
make_param_base(
  std::string const& id, std::string const& name, std::string const& default_, 
  int group, param_config const& config)
{
  param_topo result = {};
  result.id = id;
  result.name = name;
  result.group = group;
  result.config = config;
  result.default_text = default_;
  return result;
}

std::vector<item_topo>
note_name_items() 
{
  std::vector<item_topo> result;
  result.emplace_back("{BAE982FB-29FA-4E6A-B8E9-88DED2B71F97}", "C");
  result.emplace_back("{F6B7C5C1-2F00-48C3-BB2B-95F6D26C4371}", "C#");
  result.emplace_back("{80E1A69D-F1D6-4D34-A7AC-762014D3D547}", "D");
  result.emplace_back("{10D8A2F6-AECE-4089-A76F-3187756489BC}", "D#");
  result.emplace_back("{98AA08E3-F29E-41F9-AB9A-7FD9CB4C2408}", "E");
  result.emplace_back("{3B362152-0D3D-4669-91E2-0C375859C4EC}", "F");
  result.emplace_back("{F063ACFF-B00F-496C-8B29-1CA6E85B3A37}", "F#");
  result.emplace_back("{1910726E-A4A6-4430-85E4-3355340A310D}", "G");
  result.emplace_back("{9136132A-5C63-43FE-8218-D9B17C53F82E}", "G#");
  result.emplace_back("{C4E5CD80-527D-4AA8-A1CD-AAC3A5165AB5}", "A");
  result.emplace_back("{AC865BAA-5AF9-4C62-883F-140756310CC6}", "A#");
  result.emplace_back("{79231D69-2FF6-4A49-B30A-54281EA696D9}", "B");
  return result;
};

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
make_param_toggle(
  std::string const& id, std::string const& name, std::string const& default_,
  int group, param_config const& config)
{
  param_topo result(make_param_base(id, name, default_, group, config));
  result.min = 0;
  result.max = 1;
  return result;
}

param_topo
make_param_pct(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, int group, param_config const& config)
{
  param_topo result(make_param_base(id, name, default_, group, config));
  result.min = min;
  result.max = max;
  result.unit = "%";
  result.percentage = true;
  return result;
}

param_topo
make_param_step(
  std::string const& id, std::string const& name, std::string const& default_,
  int min, int max, int group, param_config const& config)
{
  param_topo result(make_param_base(id, name, default_, group, config));
  result.min = min;
  result.max = max;
  return result;
}

param_topo
make_param_list(
  std::string const& id, std::string const& name, std::string const& default_,
  std::vector<item_topo> const& items, int group, param_config const& config)
{
  param_topo result(make_param_base(id, name, default_, group, config));
  result.min = 0;
  result.max = items.size() - 1;
  return result;
}

param_topo
make_param_log(
  std::string const& id, std::string const& name, std::string const& default_, std::string const& unit,
  double min, double max, double midpoint, int group, param_config const& config)
{
  param_topo result(make_param_base(id, name, default_, group, config));
  result.min = min;
  result.max = max;
  result.unit = unit;
  return result;
}

param_config param_config_input_toggle() 
{ return { param_rate ::block, param_format::step, param_storage::num, param_display::toggle, param_direction::input }; }
param_config param_config_input_step_list() 
{ return { param_rate::block, param_format::step, param_storage::num, param_display::list, param_direction::input }; }
param_config param_config_input_step_knob() 
{ return { param_rate::block, param_format::step, param_storage::num, param_display::knob, param_direction::input }; }
param_config param_config_input_step_hslider() 
{ return { param_rate::block, param_format::step, param_storage::num, param_display::hslider, param_direction::input }; }
param_config param_config_input_step_vslider() 
{ return { param_rate::block, param_format::step, param_storage::num, param_display::vslider, param_direction::input }; }
param_config param_config_input_list_list()
{ return { param_rate::block, param_format::step, param_storage::list, param_display::list, param_direction::input }; }
param_config param_config_input_list_knob()
{ return { param_rate::block, param_format::step, param_storage::list, param_display::knob, param_direction::input }; }
param_config param_config_input_list_hslider()
{ return { param_rate::block, param_format::step, param_storage::list, param_display::hslider, param_direction::input }; }
param_config param_config_input_list_vslider()
{ return { param_rate::block, param_format::step, param_storage::list, param_display::vslider, param_direction::input }; }
param_config param_config_input_log_block_knob()
{ return { param_rate::block, param_format::log, param_storage::num, param_display::knob, param_direction::input }; }
param_config param_config_input_log_block_hslider()
{ return { param_rate::block, param_format::log, param_storage::num, param_display::hslider, param_direction::input }; }
param_config param_config_input_log_block_vslider()
{ return { param_rate::block, param_format::log, param_storage::num, param_display::vslider, param_direction::input }; }
param_config param_config_input_log_accurate_knob()
{ return { param_rate::accurate, param_format::log, param_storage::num, param_display::knob, param_direction::input }; }
param_config param_config_input_log_accurate_hslider()
{ return { param_rate::accurate, param_format::log, param_storage::num, param_display::hslider, param_direction::input }; }
param_config param_config_input_log_accurate_vslider()
{ return { param_rate::accurate, param_format::log, param_storage::num, param_display::vslider, param_direction::input }; }
param_config param_config_input_linear_block_knob()
{ return { param_rate::block, param_format::linear, param_storage::num, param_display::knob, param_direction::input }; }
param_config param_config_input_linear_block_hslider()
{ return { param_rate::block, param_format::linear, param_storage::num, param_display::hslider, param_direction::input }; }
param_config param_config_input_linear_block_vslider()
{ return { param_rate::block, param_format::linear, param_storage::num, param_display::vslider, param_direction::input }; }
param_config param_config_input_linear_accurate_knob()
{ return { param_rate::accurate, param_format::linear, param_storage::num, param_display::knob, param_direction::input }; }
param_config param_config_input_linear_accurate_hslider()
{ return { param_rate::accurate, param_format::linear, param_storage::num, param_display::hslider, param_direction::input }; }
param_config param_config_input_linear_accurate_vslider()
{ return { param_rate::accurate, param_format::linear, param_storage::num, param_display::vslider, param_direction::input }; }

}