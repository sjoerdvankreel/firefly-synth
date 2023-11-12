#include <plugin_base/desc/plugin.hpp>
#include <set>

namespace plugin_base {

plugin_desc::
plugin_desc(plugin_topo const* plugin, format_config const* config):
plugin(plugin), config(config)
{
  assert(plugin);
  assert(config);

  int param_global = 0;
  int module_global = 0;
  int midi_source_global = 0;

  for(int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    midi_mappings.topo_to_index.emplace_back();
    param_mappings.topo_to_index.emplace_back();
    if(module.dsp.stage == module_stage::input) module_voice_start++;
    if(module.dsp.stage == module_stage::input) module_output_start++;
    if(module.dsp.stage == module_stage::voice) module_output_start++;
    module_id_to_index[module.info.tag.id] = m;
    auto& param_id_mapping = param_mappings.id_to_index[module.info.tag.id];
    for(int p = 0; p < module.params.size(); p++)
      param_id_mapping[module.params[p].info.tag.id] = p;
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      midi_mappings.topo_to_index[m].emplace_back();
      param_mappings.topo_to_index[m].emplace_back();
      modules.emplace_back(module_desc(module, m, mi, module_global++, param_global, midi_source_global));
      for (int ms = 0; ms < module.midi_sources.size(); ms++)
      {
        auto const& source = module.midi_sources[ms];
        INF_ASSERT_EXEC(midi_mappings.message_to_index.insert(std::pair(source.message, midi_source_global)).second);
        midi_mappings.topo_to_index[m][mi].push_back(midi_source_global++);
      }
      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        param_mappings.topo_to_index[m][mi].emplace_back();
        for(int pi = 0; pi < param.info.slot_count; pi++)
          param_mappings.topo_to_index[m][mi][p].push_back(param_global++);
      }
    }
  }

  param_global = 0;
  midi_source_global = 0;
  for(int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      param_mapping mapping;
      mapping.param_local = p;
      mapping.module_global = m;
      mapping.topo.param_slot = param.info.slot;
      mapping.topo.param_index = param.info.topo;
      mapping.topo.module_slot = module.info.slot;
      mapping.topo.module_index = module.info.topo;
      mapping.param_global = param_global++;
      param_mappings.index_to_tag.push_back(param.info.id_hash);
      param_mappings.tag_to_index[param.info.id_hash] = param_mappings.params.size();
      param_mappings.params.push_back(std::move(mapping));
      params.push_back(&module.params[p]);
    }

    for (int ms = 0; ms < module.midi_sources.size(); ms++)
    {
      auto const& source = module.midi_sources[ms];
      midi_mapping mapping;
      mapping.midi_local = ms;
      mapping.module_global = m;
      mapping.topo.midi_index = source.info.topo;
      mapping.topo.module_slot = module.info.slot;
      mapping.topo.module_index = module.info.topo;
      mapping.midi_global = midi_source_global++;
      midi_mappings.index_to_tag.push_back(source.info.id_hash);
      midi_mappings.tag_to_index[source.info.id_hash] = midi_mappings.midi_sources.size();
      midi_mappings.midi_sources.push_back(std::move(mapping));
      midi_sources.push_back(&module.midi_sources[ms]);
    }
  }

  param_count = param_global;
  midi_count = midi_source_global;
  module_count = modules.size();
}

void
plugin_desc::validate() const
{
  int param_global = 0;
  int midi_source_global = 0;
  (void)param_global;
  (void)midi_source_global;

  std::set<int> all_hashes;
  std::set<std::string> all_ids;

  plugin->validate();
  midi_mappings.validate(*this);
  param_mappings.validate(*this);

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
    for (int ms = 0; ms < module.midi_sources.size(); ms++)
    {
      auto const& source = module.midi_sources[ms];
      assert(source.info.global == midi_source_global++);
      INF_ASSERT_EXEC(all_ids.insert(source.info.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(source.info.id_hash).second);
    }
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      assert(param.info.global == param_global++);
      INF_ASSERT_EXEC(all_ids.insert(param.info.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.info.id_hash).second);
    }
  }
}

void
plugin_midi_mappings::validate(plugin_desc const& plugin) const
{
  assert(midi_sources.size() == plugin.midi_count);
  assert(tag_to_index.size() == plugin.midi_count);
  assert(index_to_tag.size() == plugin.midi_count);
  assert(topo_to_index.size() == plugin.plugin->modules.size());

  int midi_source_global = 0;
  (void)midi_source_global;
  for (int m = 0; m < plugin.plugin->modules.size(); m++)
  {
    auto const& module = plugin.plugin->modules[m];
    (void)module;
    assert(topo_to_index[m].size() == module.info.slot_count);
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(topo_to_index[m][mi].size() == module.midi_sources.size());
      for (int ms = 0; ms < module.midi_sources.size(); ms++)
        assert(topo_to_index[m][mi][ms] == midi_source_global++);
    }
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
