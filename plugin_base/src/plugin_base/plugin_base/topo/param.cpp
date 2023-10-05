#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>

namespace plugin_base {

void
param_topo::validate(module_topo const& module) const
{
  info.validate();

  assert(0 <= section && section < module.sections.size());
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(dsp.format == param_format::plain || domain.is_real());
  assert(dsp.direction != param_direction::output || dsp.rate == param_rate::block);
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);

  domain.validate();
  gui.bindings.validate(module, info.slot_count);
  assert(dsp.direction == param_direction::input || gui.bindings.enabled.selector == nullptr);
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  gui.position.validate(module.sections[section].gui.dimension);
}

}