#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

param_desc::
param_desc(
  module_topo const& module_, int module_slot,
  param_topo const& param_, int topo, int slot, int local_, int global)
{
  local = local_;
  param = &param_;
  info.topo = topo;
  info.slot = slot;
  info.global = global;
  info.name = desc_name(param_.info, slot);
  full_name = desc_name(module_.info, module_slot) + " " + info.name;
  info.id = desc_id(module_.info, module_slot) + "-" + desc_id(param_.info, slot);
  info.id_hash = desc_id_hash(info.id.c_str());
}

void
param_desc::validate(module_desc const& module, int index) const
{
  assert(param);
  assert(local == index);
  assert(info.name.size() < full_name.size());
  info.validate(module.params.size(), param->info.slot_count);
}

}
