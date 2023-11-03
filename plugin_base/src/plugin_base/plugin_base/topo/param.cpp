#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>
#include <set>

namespace plugin_base {

void 
param_dsp::validate(int module_slot) const
{
  param_automate automate = automate_selector(module_slot);

  if (automate != param_automate::none)
    assert(direction == param_direction::input);

  if (direction == param_direction::output)
  {
    assert(rate == param_rate::block);
    assert(automate == param_automate::none);
  }

  if (rate == param_rate::accurate)
  {
    assert(automate != param_automate::none);
    assert(direction == param_direction::input);
  }

  if (can_modulate(module_slot))
  {
    assert(rate == param_rate::accurate);
    assert(direction == param_direction::input);
  }
}

void
param_topo_gui::validate(module_topo const& module, param_topo const& param) const
{
  bindings.validate(module, param.info.slot_count);
  position.validate(module.sections[section].gui.dimension);

  if (submenu.get())
    submenu->validate();
  for (int e = 0; e < bindings.enabled.params.size(); e++)
    assert(param.info.index != bindings.enabled.params[e]);
  for (int v = 0; v < bindings.visible.params.size(); v++)
    assert(param.info.index != bindings.visible.params[v]);
}

void
param_topo::validate(module_topo const& module, int index) const
{
  info.validate();
  dsp.validate(module.info.slot_count);
  domain.validate(module.info.slot_count);

  assert(info.index == index);
  assert(domain.max > domain.min);
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(0 <= gui.section && gui.section < module.sections.size());
  assert((info.slot_count == 1) == (gui.layout == param_layout::single));
  assert(dsp.direction == param_direction::input || !gui.bindings.enabled.is_bound());
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction != param_direction::output || module.dsp.stage == module_stage::output);
}

}