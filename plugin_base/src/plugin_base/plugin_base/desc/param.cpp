#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

// todo shared with module
static std::string
param_id(param_topo const& param, int slot)
{
  std::string result = param.info.tag.id;
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
  full_name = module_desc_name(module_, module_slot) + " " + name;
  id = module_desc_id(module_, module_slot) + "-" + param_id(param_, slot);
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
