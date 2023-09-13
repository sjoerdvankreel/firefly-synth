#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <set>

namespace plugin_base {

// nonnegative int, required for vst3 param tags
static int
stable_hash(std::string const& text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text.c_str());
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

static std::string
module_id(module_topo const& module, int index)
{
  std::string result = module.id;
  result += "-" + std::to_string(index);
  return result;
}

static std::string
module_name(module_topo const& module, int index)
{
  std::string result = module.name;
  if(module.count > 1) result += " " + std::to_string(index + 1);
  return result;
}

static std::string
param_id(param_topo const& param, int index)
{
  std::string result = param.id;
  result += "-" + std::to_string(index);
  return result;
}

static std::string
param_name(param_topo const& param, int index)
{
  std::string result = param.name;
  if (param.count > 1) result += " " + std::to_string(index + 1);
  return result;
}

// make sure we correctly build up the runtime descriptors
static void 
validate_desc(plugin_desc const& desc)
{
  // lets just make sure everything is globally unique across modules and parameters
  std::set<int> all_hashes;
  std::set<std::string> all_ids;

  assert(desc.global_param_index_to_param_id.size() == desc.global_param_mappings.size());
  assert(desc.param_id_to_global_param_index.size() == desc.global_param_mappings.size());
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    assert(module.topo);
    assert(module.id.size());
    assert(module.name.size());
    assert(module.params.size());
    assert(module.id_hash >= 0);
    assert(module.topo_index_in_plugin >= 0);
    assert(module.topo_index_in_plugin < desc.modules.size());
    assert(module.module_index_in_topo >= 0);
    assert(module.module_index_in_topo < module.topo->count);
    INF_ASSERT_EXEC(all_ids.insert(module.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.id_hash).second);

    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.topo);
      assert(param.id_hash >= 0);
      assert(param.id.size() > 0);
      assert(param.name.size() > 0);
      assert(param.param_index_in_topo >= 0);
      assert(param.param_index_in_topo < param.topo->count);
      assert(param.topo_index_in_module >= 0);
      assert(param.topo_index_in_module < module.params.size());
      INF_ASSERT_EXEC(all_ids.insert(param.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.id_hash).second);
    }
  }
}

// validate assumptions, not all combinations are valid
static void
validate_topo(plugin_topo const& topo)
{
  std::set<std::string> param_ids;
  std::set<std::string> module_ids;

  assert(topo.modules.size());
  assert(topo.polyphony >= 0 && topo.polyphony <= 1024);
  assert(0 < topo.gui_default_width && topo.gui_default_width <= 3840);
  assert(0 < topo.gui_aspect_ratio && topo.gui_aspect_ratio <= 21.0 / 9.0);

  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    assert(module.count > 0);
    assert(module.engine_factory);
    assert(module.id.size());
    assert(module.name.size());
    assert(module.params.size());
    assert(module.param_sections.size());
    assert(module.param_sections.size() <= module.params.size());
    INF_ASSERT_EXEC(module_ids.insert(module.id).second);

    for (int s = 0; s < module.param_sections.size(); s++)
    {
      auto const& section = module.param_sections[s];
      assert(section.name.size());
      assert(section.section >= 0);
      assert(section.section < module.param_sections.size());
    }

    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.count > 0);
      assert(param.count <= 1024);
      assert(param.id.size());
      assert(param.name.size());
      assert(param.section >= 0);
      assert(param.section < module.param_sections.size());
      assert(param.max > param.min);
      assert(param.default_text.size());

      if (param.direction == param_direction::output)
        assert(param.rate == param_rate::block);

      if (param.is_real())
      {
        if (param.percentage != param_percentage::no_unit)
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
        if (param.percentage == param_percentage::on)
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
      }
      else
        assert(param.names.size() == 0);

      if (param.type == param_type::item)
      {
        assert(param.items.size() > 0);
        assert(param.unit.size() == 0);
        assert(param.min == 0);
        assert(param.max == param.items.size() - 1);
      }
      else
        assert(param.items.size() == 0);

      INF_ASSERT_EXEC(param_ids.insert(param.id).second);
    }
  }
}

param_desc::
param_desc(
  module_topo const& module, param_topo const& param,
  int module_index_in_topo, int topo_index_in_module, int param_index_in_topo)
{
  topo = &param;
  this->param_index_in_topo = param_index_in_topo;
  this->topo_index_in_module = topo_index_in_module;
  id = module_id(module, module_index_in_topo) + "-" + param_id(param, param_index_in_topo);
  name = module_name(module, module_index_in_topo) + " " + param_name(param, param_index_in_topo);
  id_hash = stable_hash(id.c_str());
}

module_desc::
module_desc(
  module_topo const& module,
  int topo_index_in_plugin, int module_index_in_topo)
{
  topo = &module;
  this->topo_index_in_plugin = topo_index_in_plugin;
  this->module_index_in_topo = module_index_in_topo;
  id = module_id(module, module_index_in_topo);
  name = module_name(module, module_index_in_topo);
  id_hash = stable_hash(id);
  for(int p = 0; p < module.params.size(); p++)
  {
    auto const& param = module.params[p];
    for(int i = 0; i < param.count; i++)
      params.emplace_back(param_desc(module, param, module_index_in_topo, p, i));
  }
}

plugin_desc::
plugin_desc(plugin_topo_factory factory):
topo(factory())
{
  validate_topo(topo);

  for(int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    for(int i = 0; i < module.count; i++)
      modules.emplace_back(module_desc(module, m, i));
  }

  for(int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      param_mapping mapping;
      mapping.param_index_in_topo = param.param_index_in_topo;
      mapping.param_topo_index_in_module = param.topo_index_in_module;
      mapping.module_index_in_topo = module.module_index_in_topo;
      mapping.module_topo_index_in_plugin = module.topo_index_in_plugin;
      global_param_index_to_param_id.push_back(param.id_hash);
      param_id_to_global_param_index[param.id_hash] = global_param_mappings.size();
      global_param_mappings.push_back(mapping);
    }
  }

  validate_desc(*this);
}

void
plugin_desc::init_default_state(jarray4d<plain_value>& state) const
{
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    for(int mi = 0; mi < module.count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for(int pi = 0; pi < param.count; pi++)
          state[m][mi][p][pi] = param.default_plain();
      } 
  }
}

plugin_dims::
plugin_dims(plugin_topo const& plugin)
{
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    module_counts.push_back(module.count);
    module_param_counts.emplace_back();
    for(int mi = 0; mi < module.count; mi++)
    {
      module_param_counts[m].emplace_back();
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        module_param_counts[m][mi].push_back(param.count);
      }
    }
  }
}

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  for (int m = 0; plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    int cv_frames = module.output == module_output::cv ? frame_count : 0;
    int audio_frames = module.output == module_output::audio ? frame_count : 0;
    module_audio_frame_counts.emplace_back();
    module_accurate_frame_counts.emplace_back();
    module_cv_frame_counts.emplace_back(std::vector<int>(module.count, cv_frames));
    for (int mi = 0; mi < module.count; mi++)
    {
      module_accurate_frame_counts[m].emplace_back();
      module_audio_frame_counts[m].emplace_back(std::vector<int>(2, audio_frames));
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        int param_frames = param.rate == param_rate::accurate ? frame_count : 0;
        module_accurate_frame_counts[m][mi].push_back(std::vector<int>(param.count, param_frames));
      }
    }
  }
}

}
