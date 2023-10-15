#include <plugin_base/desc/plugin.hpp>
#include <set>

namespace plugin_base {

plugin_desc::
plugin_desc(plugin_topo const* plugin):
plugin(plugin)
{
  int param_global = 0;
  int module_global = 0;
  plugin->validate();

  for(int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    mappings.topo_to_index.emplace_back();
    if(module.dsp.stage == module_stage::input) module_voice_start++;
    if(module.dsp.stage == module_stage::input) module_output_start++;
    if(module.dsp.stage == module_stage::voice) module_output_start++;
    module_id_to_index[module.info.tag.id] = m;
    for(int p = 0; p < module.params.size(); p++)
      mappings.id_to_index[module.info.tag.id][module.params[p].info.tag.id] = p;
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      mappings.topo_to_index[m].emplace_back();
      modules.emplace_back(module_desc(module, m, mi, module_global++, param_global));
      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        mappings.topo_to_index[m][mi].emplace_back();
        for(int pi = 0; pi < param.info.slot_count; pi++)
          mappings.topo_to_index[m][mi][p].push_back(param_global++);
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
      mapping.module_slot = module.info.slot;
      mapping.module_topo = module.info.topo;
      mapping.param_local = p;
      mapping.param_slot = param.info.slot;
      mapping.param_topo = param.info.topo;
      mapping.param_global = param_global++;
      mappings.index_to_tag.push_back(param.info.id_hash);
      mappings.tag_to_index[param.info.id_hash] = mappings.params.size();
      mappings.params.push_back(std::move(mapping));
      params.push_back(&module.params[p]);
    }
  }

  param_count = param_global;
  module_count = modules.size();

  for(int p = 0; p < param_count; p++)
  {
    param_dependents.emplace_back();
    auto const& m = mappings.params[p];
    auto const& module = plugin->modules[m.module_topo];
    auto const& param = module.params[m.param_topo];
    if(param.domain.type != domain_type::dependent) continue;
    int dependency_index = mappings.topo_to_index[m.module_topo][m.module_slot][param.dependency_index][m.param_slot];
    param_dependents[dependency_index].push_back(p);
  }
}

void
plugin_desc::validate() const
{
  int param_global = 0;
  (void)param_global;
  std::set<int> all_hashes;
  std::set<std::string> all_ids;

  assert(param_count > 0);
  assert(module_count > 0);
  assert(module_voice_start >= 0);
  assert(params.size() == param_count);
  assert(modules.size() == module_count);
  assert(module_voice_start <= module_output_start);
  assert(module_output_start < plugin->modules.size());

  for (int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    module.validate(*this, m);
    INF_ASSERT_EXEC(all_ids.insert(module.info.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.info.id_hash).second);
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.info.global == param_global++);
      INF_ASSERT_EXEC(all_ids.insert(param.info.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.info.id_hash).second);
    }
  }

  for (int p = 0; p < param_count; p++)
  {
    auto const& m = mappings.params[p];
    auto const& this_param = param_at_mapping(m);
    if(this_param.param->dependency_index == -1) continue;
    auto const& that_index = mappings.topo_to_index[m.module_topo][m.module_slot][this_param.param->dependency_index][m.param_slot];
    auto const& dependents = param_dependents[that_index];
    assert(std::find(dependents.begin(), dependents.end(), p) != dependents.end());
  }
}

void
plugin_param_mappings::validate(plugin_desc const& plugin) const
{
  assert(params.size() == plugin.param_count);
  assert(tag_to_index.size() == plugin.param_count);
  assert(index_to_tag.size() == plugin.param_count);
  assert(id_to_index.size() == plugin.plugin->modules.size());
  assert(id_to_index.size() == plugin.plugin->modules.size());
  assert(topo_to_index.size() == plugin.plugin->modules.size());

  int param_global = 0;
  (void)param_global;
  for (int m = 0; m < plugin.plugin->modules.size(); m++)
  {
    auto const& module = plugin.plugin->modules[m];
    (void)module;
    assert(topo_to_index[m].size() == module.info.slot_count);
    assert(id_to_index.at(module.info.tag.id).size() == module.params.size());
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(topo_to_index[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        assert(topo_to_index[m][mi][p].size() == param.info.slot_count);
        for (int pi = 0; pi < param.info.slot_count; pi++)
          assert(topo_to_index[m][mi][p][pi] == param_global++);
      }
    }
  }
}

}
