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
enum { output_silence, output_mixed };
enum { param_on, param_source, param_target, param_gain };

class audio_matrix_engine:
public module_engine { 
  bool const _global;
  audio_matrix_mixer _mixer;
  jarray<float, 4>* _own_audio = {};
  std::vector<module_topo_mapping> const _sources;
  std::vector<module_topo_mapping> const _targets;

public:
  void initialize() override {}
  INF_PREVENT_ACCIDENTAL_COPY(audio_matrix_engine);
  audio_matrix_engine(bool global,
    std::vector<module_topo_mapping> const& sources,
    std::vector<module_topo_mapping> const& targets): 
    _global(global), _mixer(this), _sources(sources), _targets(targets) {}

  void process(plugin_block& block) override;
  jarray<float, 2> const& mix(plugin_block& block, int module, int slot);
};

static void
initializer(module_init_type init_type, bool global, plugin_state& state)
{
  if(init_type == module_init_type::minimal && global)
  {
    state.set_text_at(module_gaudio_matrix, 0, param_on, 0, "On");
    state.set_text_at(module_gaudio_matrix, 0, param_source, 0, "Voice");
    state.set_text_at(module_gaudio_matrix, 0, param_target, 0, "Master");
  }
  if (init_type == module_init_type::minimal && !global)
  {
    state.set_text_at(module_vaudio_matrix, 0, param_on, 0, "On");
    state.set_text_at(module_vaudio_matrix, 0, param_source, 0, "Osc 1");
    state.set_text_at(module_vaudio_matrix, 0, param_target, 0, "Voice");
  }
  if (init_type == module_init_type::default_ && global)
  {
    state.set_text_at(module_gaudio_matrix, 0, param_on, 0, "On");
    state.set_text_at(module_gaudio_matrix, 0, param_on, 1, "On");
    state.set_text_at(module_gaudio_matrix, 0, param_on, 2, "On");
    state.set_text_at(module_gaudio_matrix, 0, param_source, 0, "Voice");
    state.set_text_at(module_gaudio_matrix, 0, param_target, 0, "GFX 1");
    state.set_text_at(module_gaudio_matrix, 0, param_source, 1, "GFX 1");
    state.set_text_at(module_gaudio_matrix, 0, param_target, 1, "GFX 2");
    state.set_text_at(module_gaudio_matrix, 0, param_source, 2, "GFX 2");
    state.set_text_at(module_gaudio_matrix, 0, param_target, 2, "Master");
  }
  if (init_type == module_init_type::default_ && !global)
  {
    state.set_text_at(module_vaudio_matrix, 0, param_on, 0, "On");
    state.set_text_at(module_vaudio_matrix, 0, param_on, 1, "On");
    state.set_text_at(module_vaudio_matrix, 0, param_source, 0, "Osc 1");
    state.set_text_at(module_vaudio_matrix, 0, param_target, 0, "VFX 1");
    state.set_text_at(module_vaudio_matrix, 0, param_source, 1, "VFX 1");
    state.set_text_at(module_vaudio_matrix, 0, param_target, 1, "Voice");
  }
}

module_topo 
audio_matrix_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos, bool global,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  auto const voice_info = make_topo_info("{6EDEA9FD-901E-4B5D-9CDE-724AC5538B35}", "VAudio", module_vaudio_matrix, 1);
  auto const global_info = make_topo_info("{787CDC52-0F59-4855-A7B6-ECC1FB024742}", "GAudio", module_gaudio_matrix, 1);
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);
  int this_module = global? module_gaudio_matrix: module_vaudio_matrix;

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, 0, { 
      make_module_dsp_output(false, make_topo_info("{59AF084C-927D-4AFD-BA81-055687FF6A79}", "Silence", output_silence, 1)), 
      make_module_dsp_output(false, make_topo_info("{3EFFD54D-440A-4C91-AD4F-B1FA290208EB}", "Mixed", output_mixed, route_count)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));
  result.gui.tabbed_name = global? "Global": "Voice";

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{5DF08D18-3EB9-4A43-A76C-C56519E837A2}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, -30 } })));
  
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
  target.gui.item_enabled.bind_param({ this_module, 0, param_source, gui_item_binding::match_param_slot }, 
    [global, sm = source_matrix.mappings, tm = target_matrix.mappings](int other, int self) {
      int fx_index = global? module_gfx: module_vfx;
      if(sm[other].index == fx_index && tm[self].index == fx_index)
        return sm[other].slot < tm[self].slot;
      return true;
    });

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{C12ADFE9-1D83-439C-BCA3-30AD7B86848B}", "Gain", param_gain, route_count),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  amount.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  result.initializer = [global](auto type, auto& state) { initializer(type, global, state); };
  result.engine_factory = [global, sm = source_matrix, tm = target_matrix](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<audio_matrix_engine>(global, sm.mappings, tm.mappings); };

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
  jarray<float, 2>* result = &(*_own_audio)[output_silence][0];

  // loop through the routes
  // the first match we encounter becomes the mix target
  int this_module = _global? module_gaudio_matrix: module_vaudio_matrix;
  auto const& block_auto = block.state.all_block_automation[this_module][0];
  for (int r = 0; r < route_count; r++)
  {
    if(block_auto[param_on][r].step() == 0) continue;
    int selected_target = block_auto[param_target][r].step();
    int tm = _targets[selected_target].index;
    int tmi = _targets[selected_target].slot;
    if(tm != module || tmi != slot) continue;

    if (!activated)
    {
      result = &(*_own_audio)[output_mixed][r];
      activated = true;
    }

    // find out audio source to add
    auto& mix = *result;
    int selected_source = block_auto[param_source][r].step();
    int sm = _sources[selected_source].index;
    int smi = _sources[selected_source].slot;

    // add modulated amount to mixdown
    auto const& modulation = get_cv_matrix_mixdown(block, _global);
    auto const& gain_curve = *modulation[this_module][0][param_gain][r];
    auto const& source_audio = block.module_audio(sm, smi);
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
        mix[c][f] += gain_curve[f] * source_audio[0][0][c][f];
  }

  return *result;
}

}
