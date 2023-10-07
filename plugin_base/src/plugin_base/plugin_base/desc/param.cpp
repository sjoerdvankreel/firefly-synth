#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

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
  name = desc_name(param_.info, slot);
  full_name = desc_name(module_.info, module_slot) + " " + name;
  id = desc_id(module_.info, module_slot) + "-" + desc_id(param_.info, slot);
  id_hash = desc_id_hash(id.c_str());
}

void
param_desc::validate(module_desc const& module) const
{
  assert(param);
  // todo
  assert(id_hash >= 0);
  assert(id.size() > 0);
  assert(0 <= slot && slot < param->info.slot_count);
  assert(0 <= local && local < module.params.size());
  assert(0 <= topo && topo < module.module->params.size());
  assert(0 < name.size() && name.size() < full_name.size());
}

}
