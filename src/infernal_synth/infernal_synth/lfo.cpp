#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/support.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_on, param_sync, param_rate, param_num, param_denom };

class lfo_engine: 
public module_engine {
  float _phase;
  int const _module;

public:
  INF_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  void initialize() override { _phase = 0; }
  void process(plugin_block& block) override;
  lfo_engine(int module) : _module(module) { initialize(); }
};

module_topo
lfo_topo(plugin_base::gui_position const& pos, bool global)
{
  std::string const glfo_id = "{FAF92753-C6E4-4D78-BD7C-584EF473E29F}";
  std::string const vlfo_id = "{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}";

  std::string const id = global ? glfo_id : vlfo_id;
  int const module = global ? module_glfo : module_vlfo;
  std::string const name = global ? "Global LFO" : "Voice LFO";
  module_stage const stage = global ? module_stage::input : module_stage::voice;

  module_topo result(make_module(
    make_topo_info(id, name, module, 3),
    make_module_dsp(stage, module_output::cv, 1, 1),
    make_module_gui(gui_layout::tabbed, pos, { 1, 1 })));

  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Main"),
    make_section_gui({ 0, 0 }, { 1, 5 })));

  result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 },
      make_label_default(gui_label_contents::name))));

  result.params.emplace_back(make_param(
    make_topo_info("{2A9CAE77-13B0-406F-BA57-1A30ED2F5D80}", "Sync", param_sync, 1),
    make_param_dsp_block(param_automate::none), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 1 }, 
      make_label_default(gui_label_contents::name))));
  
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0.1, 20, 1, 2, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 }, 
      make_label_default(gui_label_contents::both))));
  rate.gui.bindings.enabled.params = { param_sync };
  rate.gui.bindings.enabled.selector = [] (auto const& vs) { return vs[0] == 0; };

  auto& num = result.params.emplace_back(make_param(
    make_topo_info("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Num", param_num, 1),
    make_param_dsp_block(param_automate::none), make_domain_step(1, 16, 1, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 3 }, 
      make_label_default(gui_label_contents::name))));
  num.gui.bindings.enabled.params = { param_sync };
  num.gui.bindings.enabled.selector = [](auto const& vs) { return vs[0] != 0; };

  auto& denom = result.params.emplace_back(make_param(
    make_topo_info("{84B58AC9-C401-4580-978C-60591AFB757B}", "Denom", param_denom, 1),
    make_param_dsp_block(param_automate::none), make_domain_step(1, 16, 4, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 4 }, 
      make_label_default(gui_label_contents::name))));
  denom.gui.bindings.enabled.params = { param_sync };
  denom.gui.bindings.enabled.selector = [](auto const& vs) { return vs[0] != 0; };

  result.engine_factory = [module](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<lfo_engine>(module); };
  return result;
}

void
lfo_engine::process(plugin_block& block)
{
  if(block.state.own_block_automation[param_on][0].step() == 0) return; 
  auto const& rate_curve = sync_or_freq_into_scratch(block, 
    _module, param_sync, param_rate, param_num, param_denom, 0);
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    block.state.own_cv[0][f] = (std::sin(2.0f * pi32 * _phase) + 1.0f) * 0.5f;
    _phase += rate_curve[f] / block.sample_rate;
    if(_phase >= 1.0f) _phase = 0.0f;
  }
}

}
