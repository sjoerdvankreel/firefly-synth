#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

void
module_midi_input::validate() const
{
  tag.validate();
  if(message == midi_msg_cc) assert(0 <= cc_number && cc_number < 128);
  else assert(cc_number == 0);
  assert(message == midi_msg_cc || message == midi_msg_pb || message == midi_msg_cp);
}

void
module_dsp::validate() const
{
  assert(0 <= scratch_count && scratch_count < topo_max);
  assert(output != module_output::none || outputs.size() == 0);
  assert(stage == module_stage::input || midi_inputs.size() == 0);
  assert(output == module_output::none || outputs.size() > 0 && outputs.size() < topo_max);
  for(int mmi = 0; mmi < midi_inputs.size(); mmi++)
    midi_inputs[mmi].validate();
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
  assert(0 <= gui.section && gui.section < plugin.gui.sections.size());
  assert(!gui.visible || (0 < sections.size() && sections.size() <= params.size()));

  for (int p = 0; p < params.size(); p++)
    params[p].validate(*this, p);
  for (int s = 0; s < sections.size(); s++)
    sections[s].validate(*this, s);

  dsp.validate();
  info.validate();
  if(!gui.visible) return;
  auto include = [](int) { return true; };
  auto always_visible = [this](int s) { return !sections[s].gui.bindings.visible.is_bound(); };
  gui.dimension.validate(vector_map(sections, [](auto const& s) { return s.gui.position; }), include, always_visible);
  gui.position.validate(plugin.gui.sections[gui.section].dimension);
}

}