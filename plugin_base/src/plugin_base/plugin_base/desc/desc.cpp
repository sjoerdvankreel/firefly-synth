#include <plugin_base/desc/desc.hpp>

#include <set>
#include <algorithm>
#include <functional>

namespace plugin_base {

static std::string
param_id(param_topo const& param, int slot)
{
  std::string result = param.info.tag.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
module_id(module_topo const& module, int slot)
{
  std::string result = module.info.tag.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
param_name(param_topo const& param, int slot)
{
  std::string result = param.info.tag.name;
  if (param.info.slot_count > 1) result += " " + std::to_string(slot + 1);
  return result;
}

static std::string
module_name(module_topo const& module, int slot)
{
  std::string result = module.info.tag.name;
  if (module.info.slot_count > 1) result += " " + std::to_string(slot + 1);
  return result;
}

// nonnegative required for vst3 param tags
static int
stable_hash(std::string const& text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text.c_str());
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

static void
validate_param_desc(module_desc const& module_desc, param_desc const& desc)
{
  assert(desc.param);
  assert(desc.id_hash >= 0);
  assert(desc.id.size() > 0);
  assert(0 <= desc.slot && desc.slot < desc.param->info.slot_count);
  assert(0 <= desc.local && desc.local < module_desc.params.size());
  assert(0 <= desc.topo && desc.topo < module_desc.module->params.size());
  assert(0 < desc.name.size() && desc.name.size() < desc.full_name.size());
}

static void
validate_plugin_desc(plugin_desc const& desc)
{
  int param_global = 0;
  std::set<int> all_hashes;
  std::set<std::string> all_ids;
  (void)param_global;

  assert(0 <= desc.module_voice_start);
  assert(desc.module_voice_start <= desc.module_output_start);
  assert(desc.module_output_start < desc.plugin->modules.size());

  assert(desc.param_count > 0);
  assert(desc.module_count > 0);
  assert(desc.params.size() == desc.param_count);
  assert(desc.modules.size() == desc.module_count);
  assert(desc.mappings.size() == desc.param_count);
  assert(desc.param_tag_to_index.size() == desc.param_count);
  assert(desc.param_index_to_tag.size() == desc.param_count);
  assert(desc.param_id_to_index.size() == desc.plugin->modules.size());
  assert(desc.module_id_to_index.size() == desc.plugin->modules.size());
  assert(desc.param_topo_to_index.size() == desc.plugin->modules.size());

  param_global = 0;
  for(int m = 0; m < desc.plugin->modules.size(); m++)
  {
    auto const& module = desc.plugin->modules[m];
    (void)module;
    assert(desc.plugin->modules[m].info.index == m);
    assert(desc.param_topo_to_index[m].size() == module.info.slot_count);
    assert(desc.param_id_to_index.at(module.info.tag.id).size() == module.params.size());
    for(int s = 0; s < module.sections.size(); s++)
      assert(module.sections[s].index == s);
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(desc.param_topo_to_index[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        assert(param.info.index == p);
        assert(desc.param_topo_to_index[m][mi][p].size() == param.info.slot_count);
        for(int pi = 0; pi < param.info.slot_count; pi++)
          assert(desc.param_topo_to_index[m][mi][p][pi] == param_global++);
      }
    }
  }

  param_global = 0;
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    assert(module.global == m);
    module.validate(desc);
    INF_ASSERT_EXEC(all_ids.insert(module.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.id_hash).second);
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      validate_param_desc(module, param);
      assert(param.global == param_global++);
      INF_ASSERT_EXEC(all_ids.insert(param.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.id_hash).second);
    }
  }
}

param_desc::
param_desc(
  module_topo const& module_, int module_slot,
  param_topo const& param_, int topo, int slot, int local, int global)
{
  param = &param_;
  this->topo = topo;
  this->slot = slot;
  this->local = local;
  this->global = global;
  name = param_name(param_, slot);
  full_name = module_name(module_, module_slot) + " " + name;
  id = module_id(module_, module_slot) + "-" + param_id(param_, slot);
  id_hash = stable_hash(id.c_str());
}

module_desc::
module_desc(
  module_topo const& module_, int topo, 
  int slot, int global, int param_global_start)
{
  module = &module_;
  this->topo = topo;
  this->slot = slot;
  this->global = global;
  id = module_id(module_, slot);
  name = module_name(module_, slot);
  id_hash = stable_hash(id);

  int param_local = 0;
  for(int p = 0; p < module_.params.size(); p++)
    for(int i = 0; i < module_.params[p].info.slot_count; i++)
      params.emplace_back(param_desc(module_, slot, 
        module_.params[p], p, i, param_local++, param_global_start++));
}

void
module_desc::validate(plugin_desc const& plugin) const
{
  assert(module);
  // TODO move to tag
  assert(id.size());
  assert(name.size());
  assert(params.size());
  assert(id_hash >= 0);
  assert(0 <= slot && slot < module->info.slot_count);
  assert(0 <= global && global < plugin.modules.size());
  assert(0 <= topo && topo < plugin.plugin->modules.size());
}

plugin_desc::
plugin_desc(std::unique_ptr<plugin_topo>&& plugin_):
plugin(std::move(plugin_))
{
  int param_global = 0;
  int module_global = 0;
  plugin->validate();

  for(int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    param_topo_to_index.emplace_back();
    if(module.dsp.stage == module_stage::input) module_voice_start++;
    if(module.dsp.stage == module_stage::input) module_output_start++;
    if(module.dsp.stage == module_stage::voice) module_output_start++;
    module_id_to_index[module.info.tag.id] = m;
    for(int p = 0; p < module.params.size(); p++)
      param_id_to_index[module.info.tag.id][module.params[p].info.tag.id] = p;
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      param_topo_to_index[m].emplace_back();
      modules.emplace_back(module_desc(module, m, mi, module_global++, param_global));
      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        param_topo_to_index[m][mi].emplace_back();
        for(int pi = 0; pi < param.info.slot_count; pi++)
          param_topo_to_index[m][mi][p].push_back(param_global++);
      }
    }
  }

  param_global = 0;
  for(int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      param_mapping mapping;
      mapping.module_global = m;
      mapping.module_slot = module.slot;
      mapping.module_topo = module.topo;
      mapping.param_local = p;
      mapping.param_slot = param.slot;
      mapping.param_topo = param.topo;
      mapping.param_global = param_global++;
      param_index_to_tag.push_back(param.id_hash);
      param_tag_to_index[param.id_hash] = mappings.size();
      mappings.push_back(std::move(mapping));
      params.push_back(&module.params[p]);
    }
  }

  param_count = param_global;
  module_count = modules.size();
  validate_plugin_desc(*this);
}

void
plugin_desc::init_defaults(jarray<plain_value, 4>& state) const
{
  for (int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    for(int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for(int pi = 0; pi < module.params[p].info.slot_count; pi++)
          state[m][mi][p][pi] = module.params[p].domain.default_plain();
  }
}

}
