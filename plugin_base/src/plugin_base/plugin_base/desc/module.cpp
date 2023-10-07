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
  this->topo = topo;
  this->slot = slot;
  this->global = global;
  id = desc_id(module_.info, slot);
  name = desc_name(module_.info, slot);
  id_hash = desc_id_hash(id);

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

}
