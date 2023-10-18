#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>

namespace plugin_base {

void 
param_dsp::validate() const
{
  if (automate == param_automate::automate)
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

  if (automate == param_automate::modulate)
  {
    assert(rate == param_rate::accurate);
    assert(direction == param_direction::input);
  }
}

void
param_topo::validate(module_topo const& module, int index) const
{
  dsp.validate();
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
    assert(domain.max > domain.min);
    assert(dependent_selector == nullptr);
    assert(dependency_indices.size() == 0);
    assert(gui.edit_type != gui_edit_type::dependent);
  }
  else
  {
    assert(dependency_indices.size() > 0);
    assert(dependent_selector != nullptr);
    assert(gui.edit_type == gui_edit_type::dependent);
    assert(dependency_indices.size() <= max_param_dependencies_count);
    assert(domain.max == max_domain_dependent_value(module, dependency_indices, dependent_selector));
    for (int d = 0; d < dependency_indices.size(); d++)
    {
      assert(dependency_indices[d] < index);
      auto const& other = module.params[dependency_indices[d]];
      assert(other.domain.min == 0);
      assert(!other.domain.is_real());
      assert(info.slot_count == other.info.slot_count);
      assert(other.dsp.direction != param_direction::output);
    }
  }

  assert(info.index == index);
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(0 <= gui.section && gui.section < module.sections.size());
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  assert(dsp.direction != param_direction::output || dependent_selector == nullptr);
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction == param_direction::input || gui.bindings.enabled.selector == nullptr);
  assert(dsp.direction != param_direction::output || module.dsp.stage == module_stage::output);
}

}