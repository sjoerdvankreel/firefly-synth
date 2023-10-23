#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { type_off, type_rate, type_sync };
enum { param_type, param_rate, param_num, param_den };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{6940ADC8-DF91-468A-8FA1-25843B1A5707}", "Off");
  result.emplace_back("{5F57863F-4157-4F53-BB02-C6693675B881}", "Rate");
  result.emplace_back("{E2692483-F48B-4037-BF74-64BB62110538}", "Sync");
  return result;
}

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
lfo_topo(int section, plugin_base::gui_position const& pos, bool global)
{
  std::string const glfo_id = "{FAF92753-C6E4-4D78-BD7C-584EF473E29F}";
  std::string const vlfo_id = "{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}";

  std::string const id = global ? glfo_id : vlfo_id;
  int const module = global ? module_glfo : module_vlfo;
  std::string const name = global ? "GLFO" : "VLFO";
  module_stage const stage = global ? module_stage::input : module_stage::voice;

  module_topo result(make_module(
    make_topo_info(id, name, module, 3),
    make_module_dsp(stage, module_output::cv, 1, 1),
    make_module_gui(section, pos, gui_layout::tabbed, { 1, 1 })));

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1, 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label_none())));
  
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0.1, 20, 1, 2, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1, 1, 2 }, 
    make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  rate.gui.bindings.enabled.params = { param_type };
  rate.gui.bindings.enabled.selector = [](auto const& vs) { return vs[0] == type_rate; };
  rate.gui.bindings.visible.params = { param_type };
  rate.gui.bindings.visible.selector = [] (auto const& vs) { return vs[0] != type_sync; };

  auto& num = result.params.emplace_back(make_param(
    make_topo_info("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Num", param_num, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 16, 1, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 1 }, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  num.gui.bindings.visible.params = { param_type };
  num.gui.bindings.visible.selector = [](auto const& vs) { return vs[0] == type_sync; };

  auto& den = result.params.emplace_back(make_param(
    make_topo_info("{84B58AC9-C401-4580-978C-60591AFB757B}", "Den", param_den, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 16, 4, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 2 }, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  den.gui.bindings.visible.params = { param_type };
  den.gui.bindings.visible.selector = [](auto const& vs) { return vs[0] == type_sync; };

  result.engine_factory = [module](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<lfo_engine>(module); };

  return result;
}

void
lfo_engine::process(plugin_block& block)
{
  int type = block.state.own_block_automation[param_type][0].step();
  if(type == type_off) return; 
  auto const& rate_curve = sync_or_freq_into_scratch(block, 
    type == type_sync, _module, param_rate, param_num, param_den, 0);
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    block.state.own_cv[0][f] = (std::sin(2.0f * pi32 * _phase) + 1.0f) * 0.5f;
    _phase += rate_curve[f] / block.sample_rate;
    if(_phase >= 1.0f) _phase = 0.0f;
  }
}

}
