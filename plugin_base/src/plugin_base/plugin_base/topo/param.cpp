#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

#include <set>

namespace plugin_base {

void 
param_dsp::validate(plugin_topo const& plugin, module_topo const& module, int module_slot) const
{
  param_automate automate = automate_selector_(module_slot);

  if (automate != param_automate::none)
    assert(direction == param_direction::input);

  if (direction == param_direction::output)
  {
    assert(rate == param_rate::block);
    assert(automate == param_automate::none);
  }

  if(rate == param_rate::block)
    assert(module.dsp.stage != module_stage::voice);
  if(rate == param_rate::voice)
    assert(module.dsp.stage == module_stage::voice);

  if (rate == param_rate::accurate)
  {
    assert(automate != param_automate::none);
    assert(direction == param_direction::input);
  }

  if (can_modulate(module_slot) || is_midi(module_slot))
  {
    assert(rate == param_rate::accurate);
    assert(direction == param_direction::input);
  }

  if (is_midi(module_slot))
  {
    assert(0 <= midi_source.module_index && midi_source.module_index < plugin.modules.size());
    assert(0 <= midi_source.module_slot && midi_source.module_slot < plugin.modules[midi_source.module_index].info.slot_count);
    assert(0 <= midi_source.midi_index && midi_source.midi_index < plugin.modules[midi_source.module_index].midi_sources.size());
  }
}

void
gui_item_binding::bind_param(param_topo_mapping param_, gui_item_binding_selector selector_)
{
  assert(selector == nullptr);
  assert(selector_ != nullptr);
  param = param_;
  selector = selector_;
}

void
param_topo_gui::validate(plugin_topo const& plugin, module_topo const& module, param_topo const& param) const
{
  assert(!item_enabled.is_bound() || is_list());
  bindings.validate(plugin, module, param.info.slot_count);
  position.validate(module.sections[section].gui.dimension);
  assert((param.gui.edit_type == gui_edit_type::output) == (param.dsp.direction == param_direction::output));

  if (submenu.get())
    submenu->validate();
  for (int e = 0; e < bindings.enabled.params.size(); e++)
    assert(param.info.index != bindings.enabled.params[e]);
  for (int v = 0; v < bindings.visible.params.size(); v++)
    assert(param.info.index != bindings.visible.params[v]);
}

void
param_topo::validate(plugin_topo const& plugin, module_topo const& module, int index) const
{
  info.validate();
  dsp.validate(plugin, module, module.info.slot_count);
  domain.validate(module.info.slot_count, info.slot_count);
  
  assert(info.index == index);
  assert(domain.max >= domain.min);
  assert(!domain.is_real() || domain.max > domain.min);
  assert(domain.is_real() || dsp.rate != param_rate::accurate);
  assert(0 <= gui.section && gui.section < module.sections.size());
  assert((info.slot_count == 1) == (gui.layout == param_layout::single));
  assert(dsp.direction == param_direction::input || !gui.bindings.enabled.is_bound());
  assert(dsp.direction == param_direction::input || !gui.bindings.global_enabled.is_bound());
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction != param_direction::output || module.dsp.stage == module_stage::output);

  if (gui.layout == param_layout::single_grid)
  {
    // assert * 2 is for the labels
    auto dimension = module.sections[gui.section].gui.dimension;
    assert(dimension.row_sizes.size() * dimension.column_sizes.size() == info.slot_count * 2);
  }
}

}