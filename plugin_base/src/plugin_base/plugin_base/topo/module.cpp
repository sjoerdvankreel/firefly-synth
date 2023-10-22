#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

void
module_topo::validate(plugin_topo const& plugin, int index) const
{
  assert(params.size());
  assert(engine_factory);
  assert(info.index == index);
  assert(0 <= dsp.scratch_count && dsp.scratch_count < topo_max);
  assert(0 < sections.size() && sections.size() <= params.size());
  assert(dsp.output != module_output::none || dsp.output_count == 0);
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  assert(0 <= gui.section && gui.section < plugin.gui.sections.size());
  assert(dsp.output == module_output::none || dsp.output_count > 0 && dsp.output_count < topo_max);

  for (int p = 0; p < params.size(); p++)
    params[p].validate(*this, p);
  for (int s = 0; s < sections.size(); s++)
    sections[s].validate(*this, s);

  auto include = [](int) { return true; };
  auto always_visible = [this](int s) { return sections[s].gui.bindings.visible.selector == nullptr; };
  gui.dimension.validate(vector_map(sections, [](auto const& s) { return s.gui.position; }), include, always_visible);
  gui.position.validate(plugin.gui.sections[gui.section].dimension);
  info.validate();
}

}