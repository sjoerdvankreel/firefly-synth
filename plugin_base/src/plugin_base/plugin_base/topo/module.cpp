#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

void
module_topo::validate(plugin_topo const& plugin) const
{
  assert(params.size());
  assert(engine_factory);
  assert(0 < sections.size() && sections.size() <= params.size());
  assert(dsp.output == module_output::none || dsp.output_count > 0);
  assert(dsp.output != module_output::none || dsp.output_count == 0);
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));

  for (int p = 0; p < params.size(); p++)
    params[p].validate(*this);
  for (int s = 0; s < sections.size(); s++)
    sections[s].validate(*this);

  auto include = [](int) { return true; };
  auto always_visible = [this](int s) { return sections[s].gui.bindings.visible.selector == nullptr; };
  gui.dimension.validate(sections, include, always_visible);
  gui.position.validate(plugin.gui.dimension);
  info.validate();
}

}