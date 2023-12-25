#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/matrix.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth { 

static int const route_count = 20;

enum { section_main };
enum { output_silence, output_mixed };
enum { param_on, param_source, param_target, param_gain, param_bal };

class audio_matrix_engine:
public module_engine { 
  bool const _global;
  audio_matrix_mixer _mixer;
  jarray<float, 4>* _own_audio = {};
  std::vector<module_topo_mapping> const _sources;
  std::vector<module_topo_mapping> const _targets;

public:
  PB_PREVENT_ACCIDENTAL_COPY(audio_matrix_engine);
  audio_matrix_engine(bool global,
    std::vector<module_topo_mapping> const& sources,
    std::vector<module_topo_mapping> const& targets): 
    _global(global), _mixer(this), _sources(sources), _targets(targets) {}

  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;
  jarray<float, 2> const& mix(plugin_block& block, int module, int slot);
};

static void
init_voice_minimal(plugin_state& state)
{
  state.set_text_at(module_vaudio_matrix, 0, param_on, 0, "On");
  state.set_text_at(module_vaudio_matrix, 0, param_source, 0, "Osc 1");
  state.set_text_at(module_vaudio_matrix, 0, param_target, 0, "V.Out");
}

static void
init_global_minimal(plugin_state& state)
{
  state.set_text_at(module_gaudio_matrix, 0, param_on, 0, "On");
  state.set_text_at(module_gaudio_matrix, 0, param_source, 0, "V.Mix");
  state.set_text_at(module_gaudio_matrix, 0, param_target, 0, "M.Out");
}

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vaudio_matrix, 0, param_on, 0, "On");
  state.set_text_at(module_vaudio_matrix, 0, param_on, 1, "On");
  state.set_text_at(module_vaudio_matrix, 0, param_on, 2, "On");
  state.set_text_at(module_vaudio_matrix, 0, param_source, 0, "Osc 1");
  state.set_text_at(module_vaudio_matrix, 0, param_target, 0, "V.FX 1");
  state.set_text_at(module_vaudio_matrix, 0, param_source, 1, "Osc 2");
  state.set_text_at(module_vaudio_matrix, 0, param_target, 1, "V.FX 1");
  state.set_text_at(module_vaudio_matrix, 0, param_source, 2, "V.FX 1");
  state.set_text_at(module_vaudio_matrix, 0, param_target, 2, "V.Out");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gaudio_matrix, 0, param_on, 0, "On");
  state.set_text_at(module_gaudio_matrix, 0, param_source, 0, "V.Mix");
  state.set_text_at(module_gaudio_matrix, 0, param_target, 0, "G.FX 1");
  state.set_text_at(module_gaudio_matrix, 0, param_on, 1, "On");
  state.set_text_at(module_gaudio_matrix, 0, param_source, 1, "G.FX 1");
  state.set_text_at(module_gaudio_matrix, 0, param_target, 1, "G.FX 2");
  state.set_text_at(module_gaudio_matrix, 0, param_on, 2, "On");
  state.set_text_at(module_gaudio_matrix, 0, param_source, 2, "G.FX 2");
  state.set_text_at(module_gaudio_matrix, 0, param_target, 2, "M.Out");
}

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, 
  param_topo_mapping const& mapping, std::vector<list_item> const& targets)
{
  auto const& m = mapping;
  int on = state.get_plain_at(m.module_index, m.module_slot, param_on, m.param_slot).step();
  if (on == 0)
  {
    // try to always paint something
    for (int r = 0; r < route_count; r++)
      if (state.get_plain_at(m.module_index, m.module_slot, param_on, r).step() != 0)
        return render_graph(state, engine, { m.module_index, m.module_slot, m.param_index, r }, targets);
    return graph_data(graph_data_type::off, {});
  }
  
  int ti = state.get_plain_at(m.module_index, m.module_slot, param_target, m.param_slot).step();
  std::vector<std::pair<float, float>> multi_stereo;
  for(int r = 0; r < route_count; r++)
    if (state.get_plain_at(m.module_index, m.module_slot, param_on, r).step() != 0)
      if (state.get_plain_at(m.module_index, m.module_slot, param_target, r).step() == ti)
      {
        float bal = state.get_plain_at(m.module_index, m.module_slot, param_bal, r).real();
        float gain = state.get_plain_at(m.module_index, m.module_slot, param_gain, r).real();
        float left = stereo_balance(0, bal) * gain;
        float right = stereo_balance(1, bal) * gain;
        multi_stereo.push_back({ left, right });
      }
  return graph_data(multi_stereo, { targets[ti].name });
}

audio_routing_audio_params
make_audio_routing_audio_params(plugin_state* state, bool global)
{
  audio_routing_audio_params result;
  result.off_value = 0;
  result.on_param = param_on;
  result.source_param = param_source;
  result.target_param = param_target;
  result.matrix_module = global ? module_gaudio_matrix : module_vaudio_matrix;
  result.sources = make_audio_matrix(make_audio_matrix_sources(state->desc().plugin, global)).mappings;
  result.targets = make_audio_matrix(make_audio_matrix_targets(state->desc().plugin, global)).mappings;
  return result;
}

module_topo 
audio_matrix_topo(
  int section, gui_colors const& colors,
  gui_position const& pos, bool global,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  auto const voice_info = make_topo_info("{6EDEA9FD-901E-4B5D-9CDE-724AC5538B35}", "Voice Audio", "V.Audio", true, module_vaudio_matrix, 1);
  auto const global_info = make_topo_info("{787CDC52-0F59-4855-A7B6-ECC1FB024742}", "Global Audio", "G.Audio", true, module_gaudio_matrix, 1);
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);
  int this_module = global? module_gaudio_matrix: module_vaudio_matrix;
  auto source_matrix = make_audio_matrix(sources);
  auto target_matrix = make_audio_matrix(targets);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, 0, { 
      make_module_dsp_output(false, make_topo_info("{59AF084C-927D-4AFD-BA81-055687FF6A79}", "Silence", output_silence, 1)), 
      make_module_dsp_output(false, make_topo_info("{3EFFD54D-440A-4C91-AD4F-B1FA290208EB}", "Mixed", output_mixed, route_count)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.graph_renderer = [tm = target_matrix.items](
    auto const& state, auto* engine, auto const& mapping) {
      return render_graph(state, engine, mapping, tm); };
  result.gui.tabbed_name = result.info.tag.short_name;
  result.default_initializer = global ? init_global_default : init_voice_default;
  result.minimal_initializer = global ? init_global_minimal : init_voice_minimal;
  result.engine_factory = [global, sm = source_matrix, tm = target_matrix](auto const& topo, int, int) {
    return std::make_unique<audio_matrix_engine>(global, sm.mappings, tm.mappings); };
  result.gui.menu_handler_factory = [](plugin_state* state) { 
    return std::make_unique<tidy_matrix_menu_handler>(state, param_on, 0, std::vector<int>({ param_target, param_source })); };

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{5DF08D18-3EB9-4A43-A76C-C56519E837A2}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 1, 1, -35, -35 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "On", param_on, route_count),
    make_param_dsp_input(!global, param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, gui_label_contents::none, make_label_none())));
  on.gui.tabular = true;

  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{842002C4-1946-47CF-9346-E3C865FA3F77}", "Source", param_source, route_count),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(source_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, gui_label_contents::value, make_label_none())));
  source.gui.tabular = true;
  source.gui.submenu = source_matrix.submenu;
  source.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  source.gui.item_enabled.bind_param({ this_module, 0, param_target, gui_item_binding::match_param_slot },
    [global, sm = source_matrix.mappings, tm = target_matrix.mappings](int other, int self) {
      int fx_index = global ? module_gfx : module_vfx;
      if (sm[self].index == fx_index && tm[other].index == fx_index)
        return sm[self].slot < tm[other].slot;
      return true;
    });

  auto default_target = global? "M.Out": "V.Out";
  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{F05208C5-F8D3-4418-ACFE-85CE247F222A}", "Target", param_target, route_count),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(target_matrix.items, default_target),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, gui_label_contents::value, make_label_none())));
  target.gui.tabular = true;
  target.gui.submenu = target_matrix.submenu;
  target.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  target.gui.item_enabled.bind_param({ this_module, 0, param_source, gui_item_binding::match_param_slot }, 
    [global, sm = source_matrix.mappings, tm = target_matrix.mappings](int other, int self) {
      int fx_index = global? module_gfx: module_vfx;
      if(sm[other].index == fx_index && tm[self].index == fx_index)
        return sm[other].slot < tm[self].slot;
      return true;
    });

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{C12ADFE9-1D83-439C-BCA3-30AD7B86848B}", "Gain", param_gain, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, gui_label_contents::value, make_label_none())));
  amount.gui.tabular = true;
  amount.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& bal = result.params.emplace_back(make_param(
    make_topo_info("{941C6961-044F-431E-8296-C5303EAFD11D}", "Bal", param_bal, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 }, gui_label_contents::value, make_label_none())));
  bal.gui.tabular = true;
  bal.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

jarray<float, 2> const& 
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
      (*result)[0].fill(block.start_frame, block.end_frame, 0.0f);
      (*result)[1].fill(block.start_frame, block.end_frame, 0.0f);
      activated = true;
    }

    // find out audio source to add
    auto& mix = *result;
    int selected_source = block_auto[param_source][r].step();
    int sm = _sources[selected_source].index;
    int smi = _sources[selected_source].slot;

    // add modulated amount to mixdown
    auto const& source_audio = block.module_audio(sm, smi);
    auto const& modulation = get_cv_matrix_mixdown(block, _global);
    auto const& bal_curve = *modulation[this_module][0][param_bal][r];
    auto const& gain_curve = *modulation[this_module][0][param_gain][r];
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
      {
        float bal = block.normalized_to_raw(this_module, param_bal, bal_curve[f]);
        mix[c][f] += gain_curve[f] * stereo_balance(c, bal) * source_audio[0][0][c][f];
      }
  }

  return *result;
}

}
