#include <plugin_base/desc/midi.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

midi_desc::
midi_desc(
  module_topo const& module_, int module_slot,
  midi_source const& source_, int topo, int local_, int global)
{
  local = local_;
  source = &source_;
  info.topo = topo;
  info.global = global;
  info.name = source_.tag.display_name;
  full_name = desc_name(module_.info, module_.info.tag.full_name, module_slot) + " " + info.name;
  info.id = desc_id(module_.info, module_slot) + "-" + source_.tag.id;
  info.id_hash = desc_id_hash(info.id.c_str());
}

void
midi_desc::validate(module_desc const& module, int index) const
{
  assert(source);
  assert(local == index);
  assert(info.topo == index);
  assert(info.name.size() < full_name.size());
  info.validate(module.midi_sources.size());
}

}
