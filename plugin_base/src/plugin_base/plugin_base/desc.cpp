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
module_id(module_topo const& module, int slot_index)
{
  std::string result = module.id;
  result += "-" + std::to_string(slot_index);
  return result;
}

static std::string
module_name(module_topo const& module, int slot_index)
{
  std::string result = module.name;
  if(module.slot_count > 1) result += " " + std::to_string(slot_index + 1);
  return result;
}

static std::string
param_id(param_topo const& param, int slot_index)
{
  std::string result = param.id;
  result += "-" + std::to_string(slot_index);
  return result;
}

static std::string
param_name(param_topo const& param, int slot_index)
{
  std::string result = param.name;
  if (param.slot_count > 1) result += " " + std::to_string(slot_index + 1);
  return result;
}

// make sure we correctly build up the runtime descriptors
static void 
validate_desc(plugin_desc const& desc)
{
  // lets just make sure everything is globally unique across modules and parameters
  std::set<int> all_hashes;
  std::set<std::string> all_ids;

  assert(desc.module_global_count > 0);
  assert(desc.modules.size() == desc.module_global_count);
  assert(desc.param_global_count > 0);
  assert(desc.param_mappings.size() == desc.param_global_count);
  assert(desc.param_id_to_index.size() == desc.param_global_count);
  assert(desc.param_index_to_id.size() == desc.param_global_count);

  int param_global_index = 0;
  (void)param_global_index;
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    assert(module.topo);
    assert(module.id.size());
    assert(module.name.size());
    assert(module.params.size());
    assert(module.id_hash >= 0);
    assert(module.global_index == m);
    assert(module.topo_index >= 0);
    assert(module.topo_index < module.topo->slot_count);
    assert(module.slot_index >= 0);
    assert(module.slot_index < desc.modules.size());
    INF_ASSERT_EXEC(all_ids.insert(module.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.id_hash).second);

    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.topo);
      assert(param.id_hash >= 0);
      assert(param.id.size() > 0);
      assert(param.short_name.size() > 0);
      assert(param.short_name.size() < param.full_name.size());
      assert(param.slot_index >= 0);
      assert(param.slot_index < param.topo->slot_count);
      assert(param.local_index >= 0);
      assert(param.local_index < module.params.size());
      assert(param.topo_index >= 0);
      assert(param.topo_index < module.topo->params.size());
      assert(param.global_index == param_global_index++);
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

  assert(topo.id.size());
  assert(topo.name.size());
  assert(topo.modules.size());
  assert(topo.version_major >= 0);
  assert(topo.version_minor >= 0);
  assert(topo.polyphony >= 0 && topo.polyphony <= 1024);
  assert(0 < topo.gui_default_width && topo.gui_default_width <= 3840);
  assert(0 < topo.gui_aspect_ratio && topo.gui_aspect_ratio <= 21.0 / 9.0);

  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    assert(module.id.size());
    assert(module.name.size());
    assert(module.params.size());
    assert(module.slot_count > 0);
    assert(module.engine_factory);
    assert(module.sections.size());
    assert(module.sections.size() <= module.params.size());
    INF_ASSERT_EXEC(module_ids.insert(module.id).second);

    for (int s = 0; s < module.sections.size(); s++)
    {
      auto const& section = module.sections[s];
      (void)section;
      assert(section.name.size());
      assert(section.section >= 0);
      assert(section.section < module.sections.size());
    }

    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      (void)param;
      assert(param.id.size());
      assert(param.name.size());
      assert(param.section >= 0);
      assert(param.slot_count > 0);
      assert(param.max > param.min);
      assert(param.slot_count <= 1024);
      assert(param.default_text.size());
      assert(param.section < module.sections.size());

      if (param.dir == param_dir::output)
        assert(param.rate == param_rate::block);

      if (param.is_real())
      {
        if (param.display != param_display::pct_no_unit)
          assert(param.unit.size() > 0);
        assert(param.min <= param.default_plain().real());
        assert(param.max >= param.default_plain().real());
      }
      else
      {
        assert((int)param.min == param.min);
        assert((int)param.max == param.max);
        assert(param.rate == param_rate::block);
        assert(param.display == param_display::normal);
        assert(param.min <= param.default_plain().step());
        assert(param.max >= param.default_plain().step());
      }

      if (param.type == param_type::log)
        assert(param.exp != 0);
      else
        assert(param.exp == 0);

      if (param.display != param_display::normal)
      {
        assert(param.type == param_type::linear);
        if (param.display == param_display::pct)
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
  module_topo const& module, int module_slot_index,
  param_topo const& param, int topo_index, int slot_index,
  int local_index, int global_index)
{
  topo = &param;
  this->topo_index = topo_index;
  this->slot_index = slot_index;
  this->local_index = local_index;
  this->global_index = global_index;
  short_name = param_name(param, slot_index);
  full_name = module_name(module, module_slot_index) + " " + short_name;
  id = module_id(module, module_slot_index) + "-" + param_id(param, slot_index);
  id_hash = stable_hash(id.c_str());
}

module_desc::
module_desc(
  module_topo const& module, int topo_index, int slot_index,
  int global_index, int param_global_index_start)
{
  topo = &module;
  int param_local_index = 0;
  this->topo_index = topo_index;
  this->slot_index = slot_index;
  this->global_index = global_index;
  id = module_id(module, slot_index);
  name = module_name(module, slot_index);
  id_hash = stable_hash(id);
  for(int p = 0; p < module.params.size(); p++)
  {
    auto const& param = module.params[p];
    for(int i = 0; i < param.slot_count; i++)
      params.emplace_back(param_desc(module, slot_index, param, p, i, param_local_index++, param_global_index_start++));
  }
}

plugin_desc::
plugin_desc(std::unique_ptr<plugin_topo>&& topo_):
topo(std::move(topo_))
{
  validate_topo(*topo);

  int param_global_index = 0;
  int module_global_index = 0;
  for(int m = 0; m < topo->modules.size(); m++)
  {
    auto const& module = topo->modules[m];
    for(int i = 0; i < module.slot_count; i++)
    {
      modules.emplace_back(module_desc(module, m, i, module_global_index++, param_global_index));
      for(int p = 0; p < module.params.size(); p++)
        param_global_index += module.params[p].slot_count;
    }
  }

  param_global_index = 0;
  for(int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      param_mapping mapping;
      mapping.module_global_index = m;
      mapping.param_local_index = p;
      mapping.param_global_index = param_global_index++;
      mapping.param_slot_index = param.slot_index;
      mapping.param_topo_index = param.topo_index;
      mapping.module_slot_index = module.slot_index;
      mapping.module_topo_index = module.topo_index;
      param_index_to_id.push_back(param.id_hash);
      param_id_to_index[param.id_hash] = param_mappings.size();
      param_mappings.push_back(std::move(mapping));
    }
  }

  module_global_count = modules.size();
  param_global_count = param_global_index;
  validate_desc(*this);
}

void
plugin_desc::init_default_state(jarray4d<plain_value>& state) const
{
  for (int m = 0; m < topo->modules.size(); m++)
  {
    auto const& module = topo->modules[m];
    for(int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for(int pi = 0; pi < param.slot_count; pi++)
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
    modules.push_back(module.slot_count);
    params.emplace_back();
    for(int mi = 0; mi < module.slot_count; mi++)
    {
      params[m].emplace_back();
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        params[m][mi].push_back(param.slot_count);
      }
    }
  }
}

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    int cv_frames = module.output == module_output::cv ? frame_count : 0;
    int audio_frames = module.output == module_output::audio ? frame_count : 0;
    audio.emplace_back();
    accurate.emplace_back();
    cv.emplace_back(std::vector<int>(module.slot_count, cv_frames));
    for (int mi = 0; mi < module.slot_count; mi++)
    {
      accurate[m].emplace_back();
      audio[m].emplace_back(std::vector<int>(2, audio_frames));
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        int param_frames = param.rate == param_rate::accurate ? frame_count : 0;
        accurate[m][mi].push_back(std::vector<int>(param.slot_count, param_frames));
      }
    }
  }
}

}
