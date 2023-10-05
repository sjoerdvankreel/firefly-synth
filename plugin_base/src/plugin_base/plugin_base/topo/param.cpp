#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>

namespace plugin_base {

void
param_topo::validate(module_topo const& module) const
{
  info.validate();
  domain.validate();
  gui.bindings.validate(module, info.slot_count);
  gui.position.validate(module.sections[section].gui.dimension);

  for (int e = 0; e < gui.bindings.enabled.params.size(); e++)
    assert(info.index != gui.bindings.enabled.params[e]);
  for (int v = 0; v < gui.bindings.visible.params.size(); v++)
    assert(info.index != gui.bindings.visible.params[v]);

  assert(0 <= section && section < module.sections.size());
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(dsp.format == param_format::plain || domain.is_real());
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  assert(dsp.direction != param_direction::output || dsp.rate == param_rate::block);
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction == param_direction::input || gui.bindings.enabled.selector == nullptr);
}

}