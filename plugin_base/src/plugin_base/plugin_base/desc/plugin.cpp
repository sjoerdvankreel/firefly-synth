#include <plugin_base/desc/plugin.hpp>

#include <set>

namespace plugin_base {

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
  validate();
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

void
plugin_desc::validate() const
{
  int param_global = 0;
  std::set<int> all_hashes;
  std::set<std::string> all_ids;
  (void)param_global;

  assert(0 <= module_voice_start);
  assert(module_voice_start <= module_output_start);
  assert(module_output_start < plugin->modules.size());

  assert(param_count > 0);
  assert(module_count > 0);
  assert(params.size() == param_count);
  assert(modules.size() == module_count);
  assert(mappings.size() == param_count);
  assert(param_tag_to_index.size() == param_count);
  assert(param_index_to_tag.size() == param_count);
  assert(param_id_to_index.size() == plugin->modules.size());
  assert(module_id_to_index.size() == plugin->modules.size());
  assert(param_topo_to_index.size() == plugin->modules.size());

  param_global = 0;
  for (int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    (void)module;
    assert(plugin->modules[m].info.index == m);
    assert(param_topo_to_index[m].size() == module.info.slot_count);
    assert(param_id_to_index.at(module.info.tag.id).size() == module.params.size());
    for (int s = 0; s < module.sections.size(); s++)
      assert(module.sections[s].index == s);
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(param_topo_to_index[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        assert(param.info.index == p);
        assert(param_topo_to_index[m][mi][p].size() == param.info.slot_count);
        for (int pi = 0; pi < param.info.slot_count; pi++)
          assert(param_topo_to_index[m][mi][p][pi] == param_global++);
      }
    }
  }

  param_global = 0;
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    assert(module.global == m);
    module.validate(*this);
    INF_ASSERT_EXEC(all_ids.insert(module.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.id_hash).second);
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      // todo move to module
      param.validate(module);
      assert(param.global == param_global++);
      INF_ASSERT_EXEC(all_ids.insert(param.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.id_hash).second);
    }
  }
}

}
