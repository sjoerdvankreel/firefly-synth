#include <plugin_base/desc/output.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

// keep an eye on this vs midi_desc, lots of duplication

output_desc::
output_desc(
  module_topo const& module_, int module_slot,
  module_dsp_output const& source_, int topo, int slot, int local_, int global)
{
  local = local_;
  source = &source_;
  info.slot = slot;
  info.topo = topo;
  info.global = global;
  info.name = source_.info.tag.display_name;
  full_name = desc_name(module_.info, module_.info.tag.full_name, module_slot) + " " + info.name;
  info.id = desc_id(module_.info, module_slot) + "-" + source_.info.tag.id;
  info.id_hash = desc_id_hash(info.id.c_str());
}

void
output_desc::validate(module_desc const& module, int index) const
{
  assert(source);
  assert(local == index);
  assert(info.topo == index);
  assert(info.name.size() < full_name.size());
  info.validate(module.output_sources.size(), source->info.slot_count);
}

}
