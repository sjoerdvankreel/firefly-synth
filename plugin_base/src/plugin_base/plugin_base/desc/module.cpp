#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

module_desc::
module_desc(
  module_topo const& module_, int topo, int slot, int global, 
  int param_global_start, int midi_source_global_start)
{
  module = &module_;
  info.topo = topo;
  info.slot = slot;
  info.global = global;
  info.id = desc_id(module_.info, slot);
  info.id_hash = desc_id_hash(info.id);
  info.name = desc_name(module_.info, module_.info.tag.full_name, slot);

  int param_local = 0;
  for(int p = 0; p < module_.params.size(); p++)
    for(int i = 0; i < module_.params[p].info.slot_count; i++)
      params.emplace_back(param_desc(module_, slot, 
        module_.params[p], p, i, param_local++, param_global_start++));

  int midi_local = 0;
  for(int ms = 0; ms < module_.midi_sources.size(); ms++)
    midi_sources.emplace_back(midi_desc(module_, slot, module_.midi_sources[ms], 
      ms, midi_local++, midi_source_global_start++));
}

void
module_desc::validate(plugin_desc const& plugin, int index) const
{
  assert(module);
  assert(info.global == index);
  assert(info.topo == module->info.index);
  assert(!module->gui.visible || params.size());
  info.validate(plugin.plugin->modules.size(), module->info.slot_count);
  for(int p = 0; p < params.size(); p++)
    params[p].validate(*this, p);
  for (int ms = 0; ms < midi_sources.size(); ms++)
    midi_sources[ms].validate(*this, ms);
}

}
