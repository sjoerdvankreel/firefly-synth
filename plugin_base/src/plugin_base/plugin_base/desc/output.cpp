#include <plugin_base/desc/output.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

// keep an eye on this vs param_desc, lots of duplication

output_desc::
output_desc(
  module_topo const& module_, int module_slot,
  module_dsp_output const& output_, int topo, int slot, int local_, int global)
{
  local = local_;
  source = &output_;
  info.topo = topo;
  info.slot = slot;
  info.global = global;
  info.name = desc_name(output_.info, output_.info.tag.full_name, slot);
  full_name = desc_name(module_.info, module_.info.tag.menu_display_name, module_slot) + " " + info.name;
  info.id = desc_id(module_.info, module_slot) + "-" + desc_id(output_.info, slot);
  info.id_hash = desc_id_hash(info.id.c_str());
}

void
output_desc::validate(module_desc const& module, int index) const
{
  assert(source);
  assert(local == index);
  assert(info.topo == source->info.index);
  assert(info.name.size() < full_name.size());
  info.validate(module.output_sources.size(), source->info.slot_count);
}

}
