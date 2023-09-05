#include <infernal.base/desc.hpp>
#include <set>

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
  result += "-" + std::to_string(module_index);
  return result;
}

static std::string
module_name(module_group_topo const& module_group, int module_index)
{
  std::string result = module_group.name;
  if(module_group.module_count > 1) result += " " + std::to_string(module_index + 1);
  return result;
}

static void 
validate(plugin_desc const& desc)
{
  std::set<int> plugin_param_hashes;
  std::set<std::string> plugin_param_ids;
  assert(desc.topo->mixdown_factory);
  assert(desc.id_to_index.size() == desc.param_mappings.size());
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.id_hash >= 0);
      assert(param.id.size() > 0);
      assert(param.name.size() > 0);
      INF_ASSERT_EXEC(plugin_param_ids.insert(param.id).second);
      INF_ASSERT_EXEC(plugin_param_hashes.insert(param.id_hash).second);
    }
  }

  std::set<std::string> param_ids;
  std::set<std::string> group_ids;
  for (int g = 0; g < desc.topo->module_groups.size(); g++)
  {
    auto const& group = desc.topo->module_groups[g];
    assert(group.id.size());
    assert(group.name.size());
    assert(group.engine_factory);
    assert(group.module_count > 0);
    assert(group.params.size() > 0);
    assert(group.param_groups.size() > 0);
    INF_ASSERT_EXEC(group_ids.insert(group.id).second);
    for (int p = 0; p < group.params.size(); p++)
    {
      auto const& param = group.params[p];
      assert(param.id.size() > 0);
      assert(param.name.size() > 0);
      assert(param.max > param.min);
      assert(param.default_text.size() > 0);
      assert(!param.percentage || param.unit == "%");
      assert(param.list.size() == 0 || param.min == 0);
      assert(param.list.size() == 0 || param.unit == "");
      assert(param.unit == "" || param.format != param_format::step);
      assert(param.format == param_format::linear || !param.percentage);
      assert((param.list.size() == 0) || param.format == param_format::step);
      assert(param.display != param_display::list || param.list.size() != 0);
      assert(param.display != param_display::toggle || (param.min == 0 && param.max == 1));
      assert(param.rate == param_rate::block || param.format != param_format::step);
      assert(param.display != param_display::list || param.format == param_format::step);
      assert(param.display != param_display::toggle || param.format == param_format::step);
      assert((param.direction == param_direction::input) || (param.rate == param_rate::block));
      INF_ASSERT_EXEC(param_ids.insert(param.id).second);
    }
  }
}

param_desc::
param_desc(module_group_topo const& module_group, int module_index, param_topo const& param)
{
  topo = &param;
  id = module_id(module_group, module_index) + "-" + param.id;
  name = module_name(module_group, module_index) + " " + param.name;
  id_hash = hash(id.c_str());
}

module_desc::
module_desc(module_group_topo const& module_group, int module_index)
{
  topo = &module_group;
  name = module_name(module_group, module_index);
  for(int i = 0; i < module_group.params.size(); i++)
    params.emplace_back(param_desc(module_group, module_index, module_group.params[i]));
}

plugin_desc::
plugin_desc(plugin_topo const& plugin)
{
  topo = &plugin;
  int plugin_param_index = 0;
  std::set<std::string> param_ids;
  for(int g = 0; g < plugin.module_groups.size(); g++)
  {
    auto const& group = plugin.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
    {
      modules.emplace_back(module_desc(group, m));
      auto& module = modules[modules.size() - 1];
      for(int p = 0; p < group.params.size(); p++)
      {
        param_mapping mapping;
        mapping.group = g;
        mapping.param = p;
        mapping.module = m;
        param_mappings.push_back(mapping);
        id_to_index[module.params[p].id_hash] = plugin_param_index++;
        INF_ASSERT_EXEC(param_ids.insert(module.params[p].id).second);
      }
    }
  }
  validate(*this);
}

plugin_dims::
plugin_dims(plugin_topo const& plugin)
{
  for (int g = 0; g < plugin.module_groups.size(); g++)
  {
    auto const& group = plugin.module_groups[g];
    module_counts.push_back(group.module_count);
    module_param_counts.emplace_back(std::vector<int>(group.module_count, group.params.size()));
  }
}

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  int group_count = plugin.module_groups.size();
  for (int g = 0; g < group_count; g++)
  {
    auto const& group = plugin.module_groups[g];
    int cv_frames = group.output == module_output::cv ? frame_count : 0;
    int audio_frames = group.output == module_output::audio ? frame_count : 0;
    module_audio_frame_counts.emplace_back();
    module_accurate_frame_counts.emplace_back();
    module_cv_frame_counts.emplace_back(std::vector<int>(group.module_count, cv_frames));
    for(int m = 0; m < group.module_count; m++)
    {
      module_accurate_frame_counts[g].emplace_back();
      module_audio_frame_counts[g].emplace_back(std::vector<int>(2, audio_frames));
      for(int p = 0; p < group.params.size(); p++)
      {
        int param_frames = group.params[p].rate == param_rate::accurate ? frame_count : 0;
        module_accurate_frame_counts[g][m].push_back(param_frames);
      }
    }
  }
}

}
