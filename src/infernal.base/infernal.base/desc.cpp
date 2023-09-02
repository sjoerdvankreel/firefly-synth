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

plugin_dimensions::
plugin_dimensions(plugin_topo const& plugin)
{
  for (int g = 0; g < plugin.module_groups.size(); g++)
  {
    auto const& group = plugin.module_groups[g];
    module_counts.push_back(group.module_count);
    module_param_counts.emplace_back(std::vector<int>(group.module_count, group.params.size()));
    module_channel_counts.emplace_back(std::vector<int>(group.module_count, plugin.channel_count));
  }
}

plugin_frame_dimensions::
plugin_frame_dimensions(plugin_topo const& plugin, int frame_count)
{
  int group_count = plugin.module_groups.size();
  for (int g = 0; g < group_count; g++)
  {
    auto const& group = plugin.module_groups[g];
    module_param_frame_counts.emplace_back();
    module_channel_frame_counts.emplace_back();
    module_frame_counts.emplace_back(std::vector<int>(group.module_count, frame_count));
    for (int m = 0; m < group.module_count; m++)
    {
      module_param_frame_counts[g].emplace_back(std::vector<int>(group.params.size(), frame_count));
      module_channel_frame_counts[g].emplace_back(std::vector<int>(plugin.channel_count, frame_count));
    }
  }
}

}
