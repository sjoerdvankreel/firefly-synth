#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>

#include <infernal_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

static int constexpr route_count = 6;

enum { section_main };
enum { param_on, param_source, param_target, param_amount };

class audio_matrix_engine:
public module_engine { 
public:
  void initialize() override {}
  void process(plugin_block& block) override {}
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(audio_matrix_engine);
};

module_topo 
audio_matrix_topo(
  int section,
  plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  module_topo result(make_module(
    make_topo_info("{196C3744-C766-46EF-BFB8-9FB4FEBC7810}", "Audio", module_audio_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::none, 0, 0),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.engine_factory = [](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<audio_matrix_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{5DF08D18-3EB9-4A43-A76C-C56519E837A2}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, -30 } })));
  
  // todo enable first
  result.params.emplace_back(make_param(
    make_topo_info("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "On", param_on, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));

  auto source_matrix = make_module_matrix(sources);
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{842002C4-1946-47CF-9346-E3C865FA3F77}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(source_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  source.gui.submenu = source_matrix.submenu;

  auto target_matrix = make_module_matrix(targets);
  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{F05208C5-F8D3-4418-ACFE-85CE247F222A}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(target_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  target.gui.submenu = target_matrix.submenu;

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{C12ADFE9-1D83-439C-BCA3-30AD7B86848B}", "Amount", param_amount, route_count),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  amount.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

}
