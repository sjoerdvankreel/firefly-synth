#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

module_desc::
module_desc(
  module_topo const& module_, int topo, 
  int slot, int global, int param_global_start)
{
  module = &module_;
  info.topo = topo;
  info.slot = slot;
  info.global = global;
  info.id = desc_id(module_.info, slot);
  info.name = desc_name(module_.info, slot);
  info.id_hash = desc_id_hash(info.id);

  int param_local = 0;
  for(int p = 0; p < module_.params.size(); p++)
    for(int i = 0; i < module_.params[p].info.slot_count; i++)
      params.emplace_back(param_desc(module_, slot, 
        module_.params[p], p, i, param_local++, param_global_start++));
}

void
module_desc::validate(plugin_desc const& plugin, int index) const
{
  assert(module);
  assert(params.size());
  assert(info.global == index);
  info.validate(plugin.plugin->modules.size(), module->info.slot_count);
  for(int p = 0; p < params.size(); p++)
    params[p].validate(*this, p);
}

}
