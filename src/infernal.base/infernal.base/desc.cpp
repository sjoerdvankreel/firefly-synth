#include <infernal.base/desc.hpp>

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
