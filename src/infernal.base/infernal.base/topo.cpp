#include <infernal.base/topo.hpp>
#include <sstream>
#include <iomanip>

namespace infernal::base {

static int
hash(std::string const& text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text.c_str());
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

param_value 
param_topo::default_value() const
{
  param_value result;
  INF_ASSERT_EXEC(from_text(default_text, result));
  return result;
}

std::string 
param_topo::to_text(param_value value) const
{
  std::ostringstream stream;
  switch (format)
  {
  case param_format::step:
    stream << value.step;
    break;
  case param_format::log:
  case param_format::linear: 
    stream << std::setprecision(precision) << value.real;
    break;
  default:
    assert(false);
    break;
  }
  std::string result = stream.str();
  if(unit.size() > 0) result += " " + unit;
  return result;
}

bool 
param_topo::from_text(std::string const& text, param_value& value) const
{
  std::istringstream stream(text);
  switch (format)
  {
  case param_format::step:
    value.step = std::numeric_limits<int>::max();
    stream >> value.step;
    return min <= value.step && value.step <= max;
  case param_format::log:
  case param_format::linear:
    value.real = std::numeric_limits<float>::max();
    stream >> value.real;
    return min <= value.real && value.real <= max;
  default:
    assert(false);
    return false;
  }
}

static std::string
module_id(module_group_topo const& module_group, int module_index)
{
  std::string result = module_group.id;
  result += "-" + module_index;
  return result;
}

static std::string
module_name(module_group_topo const& module_group, int module_index)
{
  std::string result = module_group.name;
  if(module_group.module_count > 1) result += std::to_string(module_index + 1);
  return result;
}

param_desc::
param_desc(module_group_topo const& module_group, int module_index, param_topo const& param)
{
  id = module_id(module_group, module_index) + "-" + param.id;
  name = module_name(module_group, module_index) + " " + param.name;
  id_hash = hash(id.c_str());
}

module_desc::
module_desc(module_group_topo const& module_group, int module_index)
{
  name = module_name(module_group, module_index);
  for(int i = 0; i < module_group.params.size(); i++)
    params.emplace_back(param_desc(module_group, module_index, module_group.params[i]));
}

plugin_desc::
plugin_desc(plugin_topo const& plugin)
{
  for(int g = 0; g < plugin.module_groups.size(); g++)
    for(int m = 0; m < plugin.module_groups[g].module_count; m++)
    {
      modules.emplace_back(module_desc(plugin.module_groups[g], m));
      for(int p = 0; p < plugin.module_groups[m].params.size(); p++)
      {
        param_mapping mapping;
        mapping.param_index = p;
        mapping.module_group = g;
        mapping.module_index = m;
        param_mappings.push_back(mapping);
      }
    }
}

}
