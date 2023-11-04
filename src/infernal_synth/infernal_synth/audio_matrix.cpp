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
  audio_matrix_mixer _mixer;
  jarray<float, 3>* _own_audio = {};
  std::vector<matrix_module_mapping> const _sources;
  std::vector<matrix_module_mapping> const _targets;
public:
  void initialize() override {}
  INF_PREVENT_ACCIDENTAL_COPY(audio_matrix_engine);
  audio_matrix_engine(
    std::vector<matrix_module_mapping> const& sources, 
    std::vector<matrix_module_mapping> const& targets): 
    _mixer(this), _sources(sources), _targets(targets) {}

  void process(plugin_block& block) override;
  jarray<float, 2> const& mix(plugin_block& block, int module, int slot);
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
    make_module_dsp(module_stage::voice, module_output::audio, route_count + 1, 0),
    make_module_gui(section, colors, pos, { 1, 1 })));

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

  result.engine_factory = [sm = source_matrix, tm = target_matrix](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<audio_matrix_engine>(sm.mappings, tm.mappings); };

  return result;
}

plugin_base::jarray<float, 2> const& 
audio_matrix_mixer::mix(plugin_block& block, int module, int slot)
{ return _engine->mix(block, module, slot); }

void 
audio_matrix_engine::process(plugin_block& block) 
{ 
  // need to capture own audio here because when we start 
  // mixing "own" does not refer to us but to the caller
  *block.state.own_context = &_mixer; 
  _own_audio = &block.state.own_audio; 
}

jarray<float, 2> const& 
audio_matrix_engine::mix(plugin_block& block, int module, int slot)
{
  // audio 0 is silence
  bool activated = false;
  jarray<float, 2>* result = &(*_own_audio)[0];

  // loop through the routes
  // the first match we encounter becomes the mix target
  auto const& block_auto = block.state.own_block_automation;
  for (int r = 0; r < route_count; r++)
  {
    if(block_auto[param_on][r].step() == 0) continue;
    int selected_target = block_auto[param_target][r].step();
    int tm = _sources[selected_target].topo;
    int tmi = _sources[selected_target].slot;
    if(tm != module || tmi != slot) continue;

    if (!activated)
    {
      result = &(*_own_audio)[r + 1];
      activated = true;
    }

    // find out audio source to add
    auto& mix = *result;
    int selected_source = block_auto[param_source][r].step();
    int sm = _sources[selected_source].topo;
    int smi = _sources[selected_source].slot;

    // add modulated amount to mixdown
    auto const& modulation = get_cv_matrix_mixdown(block);
    auto const& amount_curve = *modulation[module_audio_matrix][0][param_amount][r];
    if(block.plugin.modules[sm].dsp.stage == module_stage::voice)
      for(int c = 0; c < 2; c++)
        for(int f = block.start_frame; f < block.end_frame; f++)
          mix[c][f] += amount_curve[f] * block.voice->all_audio[sm][smi][0][c][f];
    else
      for (int c = 0; c < 2; c++)
        for (int f = block.start_frame; f < block.end_frame; f++)
          mix[c][f] += amount_curve[f] * block.state.all_global_audio[sm][smi][0][c][f];
  }

  return *result;
}

}
