#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

void
module_dsp::validate() const
{
  assert(0 <= scratch_count && scratch_count < topo_max);
  assert(output != module_output::none || outputs.size() == 0);
  assert(output == module_output::none || outputs.size() > 0 && outputs.size() < topo_max);
  for (int o = 0; o < outputs.size(); o++)
  {
    outputs[o].info.validate();
    assert(outputs[o].info.index == o);
    assert(output == module_output::cv || !outputs[o].is_modulation_source);
  }
}

void
module_topo::validate(plugin_topo const& plugin, int index) const
{
  assert(engine_factory);
  assert(info.index == index);
  assert(!gui.visible || params.size());
  assert(midi_sources.size() == 0 || info.slot_count == 1);
  assert(midi_sources.size() == 0 || dsp.stage == module_stage::input);
  assert(0 <= gui.section && gui.section < plugin.gui.module_sections.size());
  assert(!gui.visible || (0 < sections.size() && sections.size() <= params.size()));

  for (int p = 0; p < params.size(); p++)
    params[p].validate(plugin, *this, p);
  for (int s = 0; s < sections.size(); s++)
    sections[s].validate(*this, s);
  for (int ms = 0; ms < midi_sources.size(); ms++)
    midi_sources[ms].validate();

  dsp.validate();
  info.validate();
  if(!gui.visible) return;
  auto include = [](int) { return true; };
  auto always_visible = [this](int s) { return !sections[s].gui.bindings.visible.is_bound(); };
  gui.dimension.validate(vector_map(sections, [](auto const& s) { return s.gui.position; }), include, always_visible);
  gui.position.validate(plugin.gui.module_sections[gui.section].dimension);
}

}