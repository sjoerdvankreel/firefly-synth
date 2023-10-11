#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>

namespace plugin_base {

void
param_topo::validate(module_topo const& module, int index) const
{
  info.validate();
  domain.validate();
  gui.bindings.validate(module, info.slot_count);
  gui.position.validate(module.sections[gui.section].gui.dimension);

  for (int e = 0; e < gui.bindings.enabled.params.size(); e++)
    assert(info.index != gui.bindings.enabled.params[e]);
  for (int v = 0; v < gui.bindings.visible.params.size(); v++)
    assert(info.index != gui.bindings.visible.params[v]);

  if (domain.type != domain_type::dependent)
  {
    assert(dependent_index == 0);
    assert(dependents.size() == 0);
    assert(domain.max > domain.min);
    assert(gui.edit_type != gui_edit_type::dependent);
  }
  else
  {
    assert(dependent_index >= 0);
    assert(dependents.size() > 1);
    assert(gui.edit_type == gui_edit_type::dependent);
    assert(info.slot_count == module.params[dependent_index].info.slot_count);
    for(int i = 0; i < dependents.size(); i++)
    {
      dependents[i].validate();
      assert(dependents[i].min == 0);
      assert(dependents[i].type == domain_type::item 
          || dependents[i].type == domain_type::name 
          || dependents[i].type == domain_type::step);
    }
  }

  assert(info.index == index);
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(0 <= gui.section && gui.section < module.sections.size());
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  assert(dsp.direction != param_direction::output || dsp.rate == param_rate::block);
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction == param_direction::input || gui.bindings.enabled.selector == nullptr);
}

}