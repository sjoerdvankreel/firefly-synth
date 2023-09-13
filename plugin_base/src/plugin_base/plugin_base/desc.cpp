#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <set>

namespace plugin_base {

// nonnegative int from parameter id
static int
param_hash(std::string const& text)
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

// validate assumptions, not all combinations are valid
static void 
validate(plugin_desc const& desc)
{
  std::set<int> plugin_param_hashes;
  std::set<std::string> plugin_param_ids;
 
  assert(0 < desc.topo.gui_default_width && desc.topo.gui_default_width <= 3840);
  assert(0 < desc.topo.gui_aspect_ratio && desc.topo.gui_aspect_ratio <= 21.0 / 9.0);
  assert(desc.id_to_index.size() == desc.param_mappings.size());
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    assert(module.group_in_plugin >= 0);
    assert(module.module_in_group >= 0);
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.id_hash >= 0);
      assert(param.id.size() > 0);
      assert(param.name.size() > 0);
      assert(param.index_in_plugin >= 0);
      INF_ASSERT_EXEC(plugin_param_ids.insert(param.id).second);
      INF_ASSERT_EXEC(plugin_param_hashes.insert(param.id_hash).second);
    }
  }
 
  std::set<std::string> param_ids;
  std::set<std::string> group_ids;
  for (int g = 0; g < desc.topo.module_groups.size(); g++)
  {
    auto const& group = desc.topo.module_groups[g];
    assert(group.id.size());
    assert(group.name.size());
    assert(group.engine_factory);
    assert(group.module_count > 0);
    assert(group.params.size() > 0);
    assert(group.param_sections.size() > 0);
    INF_ASSERT_EXEC(group_ids.insert(group.id).second);
    for (int p = 0; p < group.params.size(); p++)
    {
      auto const& param = group.params[p];

      assert(param.id.size());
      assert(param.name.size());
      assert(param.section >= 0);
      assert(param.max > param.min);
      assert(param.default_text.size());

      if (param.direction == param_direction::output)
        assert(param.rate == param_rate::block);
      
      if (param.is_real())
      {
        if(param.percentage != param_percentage::no_unit)
          assert(param.unit.size() > 0);
        assert(param.min <= param.default_plain().real());
        assert(param.max >= param.default_plain().real());
      }
      else 
      {
        assert((int)param.min == param.min);
        assert((int)param.max == param.max);
        assert(param.rate == param_rate::block);
        assert(param.percentage == param_percentage::off);
        assert(param.min <= param.default_plain().step());
        assert(param.max >= param.default_plain().step());
      }

      if (param.type == param_type::log)
        assert(param.exp != 0);
      else
        assert(param.exp == 0);

      if (param.percentage != param_percentage::off)
      {
        assert(param.type == param_type::linear);
        if(param.percentage == param_percentage::on)
          assert(param.unit == "%");
      }

      if (param.edit == param_edit::toggle)
      {
        assert(param.min == 0);
        assert(param.max == 1);
        assert(param.type == param_type::step);
      }
      
      if (param.type == param_type::name)
      {
        assert(param.names.size() > 0);
        assert(param.unit.size() == 0);
        assert(param.min == 0);
        assert(param.max == param.names.size() - 1);
      } else
        assert(param.names.size() == 0);

      if (param.type == param_type::item)
      {
        assert(param.items.size() > 0);
        assert(param.unit.size() == 0);
        assert(param.min == 0);
        assert(param.max == param.items.size() - 1);
      } else
        assert(param.items.size() == 0);

      INF_ASSERT_EXEC(param_ids.insert(param.id).second);
    }
  }
}

param_desc::
param_desc(
  module_group_topo const& module_group, int module_in_group, 
  param_topo const& param, int index_in_plugin)
{
  topo = &param;
  this->index_in_plugin = index_in_plugin;
  id = module_id(module_group, module_in_group) + "-" + param.id;
  name = module_name(module_group, module_in_group) + " " + param.name;
  id_hash = param_hash(id.c_str());
}

module_desc::
module_desc(
  module_group_topo const& module_group, int group_in_plugin, 
  int module_in_group, int first_param_index_in_plugin)
{
  topo = &module_group;
  this->group_in_plugin = group_in_plugin;
  this->module_in_group = module_in_group;
  name = module_name(module_group, module_in_group);
  for(int i = 0; i < module_group.params.size(); i++)
    params.emplace_back(param_desc(module_group, module_in_group, module_group.params[i], first_param_index_in_plugin + i));
}

plugin_desc::
plugin_desc(plugin_topo_factory factory):
topo(factory())
{
  int plugin_param_index = 0;
  int plugin_module_index = 0;
  std::set<std::string> param_ids;
  for(int g = 0; g < topo.module_groups.size(); g++)
  {
    auto const& group = topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
    {
      modules.emplace_back(module_desc(group, g, m, plugin_param_index));
      auto& module = modules[modules.size() - 1];
      for(int p = 0; p < group.params.size(); p++)
      {
        param_mapping mapping;
        mapping.group_in_plugin = g;
        mapping.param_in_module = p;
        mapping.module_in_group = m;
        mapping.module_in_plugin = plugin_module_index;
        param_mappings.push_back(mapping);
        index_to_id.push_back(module.params[p].id_hash);
        id_to_index[module.params[p].id_hash] = plugin_param_index++;
        INF_ASSERT_EXEC(param_ids.insert(module.params[p].id).second);
      }
      plugin_module_index++;
    }
  }
  validate(*this);
}

void
plugin_desc::init_default_state(jarray3d<plain_value>& state) const
{
  for (int g = 0; g < topo.module_groups.size(); g++)
  {
    auto const& group = topo.module_groups[g];
    for (int m = 0; m < group.module_count; m++)
      for (int p = 0; p < group.params.size(); p++)
        state[g][m][p] = group.params[p].default_plain();
  }
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
