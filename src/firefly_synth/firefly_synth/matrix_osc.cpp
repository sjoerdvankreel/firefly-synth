#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth { 

enum { section_am, section_fm };
enum { 
  param_am_on, param_am_source, param_am_target, param_am_amt, param_am_ring,
  param_fm_on, param_fm_source, param_fm_target, param_fm_idx, param_fm_dly
};

static int const route_count = 8;
extern int const osc_param_uni_voices;

std::unique_ptr<graph_engine> make_osc_graph_engine(plugin_desc const* desc);
std::vector<graph_data> render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot, bool for_osc_matrix);

class osc_matrix_engine:
public module_engine { 
  jarray<float, 3> _no_fm = {};
  osc_matrix_context _context = {};
  jarray<float, 4>* _own_audio = {};
  osc_matrix_am_modulator _am_modulator;
  osc_matrix_fm_modulator _fm_modulator;
public:
  osc_matrix_engine(int max_frame_count);
  PB_PREVENT_ACCIDENTAL_COPY(osc_matrix_engine);

  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;

  jarray<float, 3> const& modulate_am(
    plugin_block& block, int slot, 
    cv_matrix_mixdown const* cv_modulation);
  jarray<float, 3> const& modulate_fm(
    plugin_block& block, int slot,
    cv_matrix_mixdown const* cv_modulation);
};

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int max_osc = 0;
  std::vector<float> result_l;
  std::vector<float> result_r;
  for(int r = 0; r < route_count; r++)
    if(state.get_plain_at(module_osc_matrix, 0, param_am_on, r).step() != 0)
      max_osc = std::max(max_osc, state.get_plain_at(module_osc_matrix, 0, param_am_target, r).step());
  for (int r = 0; r < route_count; r++)
    if (state.get_plain_at(module_osc_matrix, 0, param_fm_on, r).step() != 0)
      max_osc = std::max(max_osc, state.get_plain_at(module_osc_matrix, 0, param_fm_target, r).step());
  auto graphs(render_osc_graphs(state, engine, max_osc, true));
  for (int mi = 0; mi <= max_osc; mi++)
  {
    result_l.insert(result_l.end(), graphs[mi].audio()[0].cbegin(), graphs[mi].audio()[0].cend());
    result_r.insert(result_r.end(), graphs[mi].audio()[1].cbegin(), graphs[mi].audio()[1].cend());
  }
  std::vector<std::string> partitions;
  for(int i = 0; i <= max_osc; i++)
    partitions.push_back(std::to_string(i + 1));
  return graph_data(jarray<float, 2>(std::vector<jarray<float, 1>> { 
    jarray<float, 1>(result_l), jarray<float, 1>(result_r) }), 1.0f, partitions);
}

audio_routing_audio_params
make_audio_routing_osc_mod_params(plugin_state* state)
{
  audio_routing_audio_params result;
  result.off_value = 0;
  result.matrix_section_count = 2;
  result.matrix_module = module_osc_matrix;
  result.matrix_on_params = { param_am_on, param_fm_on };
  result.matrix_source_params = { param_am_source, param_fm_source };
  result.matrix_target_params = { param_am_target, param_fm_target };
  result.sources = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }, 0).mappings;
  result.targets = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }, 0).mappings;
  return result;
}

module_topo 
osc_matrix_topo(int section, gui_colors const& colors, gui_position const& pos, plugin_topo const* plugin)
{
  auto osc_matrix = make_audio_matrix({ &plugin->modules[module_osc] }, 0);

  std::vector<module_dsp_output> outputs;
  for(int r = 0; r < route_count; r++)
    outputs.push_back(make_module_dsp_output(false, make_topo_info(
      "{1DABDF9D-E777-44FF-9720-3B09AAF07C6D}-" + std::to_string(r), "AM", r, max_unison_voices + 1)));
  for (int r = 0; r < route_count; r++)
    outputs.push_back(make_module_dsp_output(false, make_topo_info(
      "{69C52C17-747D-4093-8194-43CB9527A0DE}-" + std::to_string(r), "FM", route_count + r, max_unison_voices + 1)));
  module_topo result(make_module(
    make_topo_info("{8024F4DC-5BFC-4C3D-8E3E-C9D706787362}", "Osc Mod", "Osc Mod", true, true, module_osc_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::audio, 0, outputs),
    make_module_gui(section, colors, pos, { 2, 1 })));

  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.tabbed_name = result.info.tag.name;
  result.engine_factory = [](auto const& topo, int sr, int max_frame_count) { return std::make_unique<osc_matrix_engine>(max_frame_count); };
  result.gui.menu_handler_factory = [](plugin_state* state) { return std::make_unique<tidy_matrix_menu_handler>(
    state, 2, param_am_on, 0, std::vector<std::vector<int>>({{ param_am_target, param_am_source }, { param_fm_target, param_fm_source } })); };

  auto& am = result.sections.emplace_back(make_param_section(section_am,
    make_topo_tag("{A48C0675-C020-4D05-A384-EF2B8CA8A066}", "AM"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 1, 1, 1, 1 } })));
  am.gui.scroll_mode = gui_scroll_mode::vertical;  
  auto& am_on = result.params.emplace_back(make_param(
    make_topo_info("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "AM", "AM", true, true, param_am_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_am, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  am_on.gui.tabular = true;
  am_on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, 2, 0, route_count, 1); };
  auto& am_source = result.params.emplace_back(make_param(
    make_topo_info("{1D8F3294-2463-470D-853B-561E8228467A}", "Source", "Source", true, true, param_am_source, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, ""),
    make_param_gui(section_am, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  am_source.gui.tabular = true;
  am_source.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_source.gui.item_enabled.bind_param({ module_osc_matrix, 0, param_am_target, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[self].slot <= osc[other].slot; });
  auto& am_target = result.params.emplace_back(make_param(
    make_topo_info("{1AF0E66A-ADB5-40F4-A4E1-9F31941171E2}", "Target", "Target", true, true, param_am_target, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, "Osc 2"),
    make_param_gui(section_am, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  am_target.gui.tabular = true;
  am_target.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_target.gui.item_enabled.bind_param({ module_osc_matrix, 0, param_am_source, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) { 
      return osc[other].slot <= osc[self].slot; });
  auto& am_amount = result.params.emplace_back(make_param(
    make_topo_info("{A1A7298E-542D-4C2F-9B26-C1AF7213D095}", "Amt", "Amt", true, true, param_am_amt, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui(section_am, gui_edit_type::hslider, param_layout::vertical, { 0, 3 }, make_label_none())));
  am_amount.gui.tabular = true;
  am_amount.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  auto& am_ring = result.params.emplace_back(make_param(
    make_topo_info("{3DF51ADC-9882-4F95-AF4E-5208EB14E645}", "Ring", "Ring", true, true, param_am_ring, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_am, gui_edit_type::hslider, param_layout::vertical, { 0, 4 }, make_label_none())));
  am_ring.gui.tabular = true;
  am_ring.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& fm = result.sections.emplace_back(make_param_section(section_fm,
    make_topo_tag("{1B39A828-3429-4245-BF07-551C17A78341}", "FM"),
    make_param_section_gui({ 1, 0 }, { { 1 }, { -25, 1, 1, 1, 1 } })));
  fm.gui.scroll_mode = gui_scroll_mode::vertical;
  auto& fm_on = result.params.emplace_back(make_param(
    make_topo_info("{02112C80-D1E9-409E-A9FB-6DCA34F5CABA}", "FM", "FM", true, true, param_fm_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_fm, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  fm_on.gui.tabular = true;
  fm_on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, 2, 1, route_count, 1); };
  auto& fm_source = result.params.emplace_back(make_param(
    make_topo_info("{61E9C704-E704-4669-9DC3-D3AA9FD6A952}", "Source", "Source", true, true, param_fm_source, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, ""),
    make_param_gui(section_fm, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  fm_source.gui.tabular = true;
  fm_source.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_source.gui.item_enabled.bind_param({ module_osc_matrix, 0, param_fm_target, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[self].slot <= osc[other].slot; });
  auto& fm_target = result.params.emplace_back(make_param(
    make_topo_info("{DBDD28D6-46B9-4F9A-9682-66E68A261B87}", "Target", "Target", true, true, param_fm_target, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, "Osc 2"),
    make_param_gui(section_fm, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  fm_target.gui.tabular = true;
  fm_target.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_target.gui.item_enabled.bind_param({ module_osc_matrix, 0, param_fm_source, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[other].slot <= osc[self].slot; });
  auto& fm_amount = result.params.emplace_back(make_param(
    make_topo_info("{444B0AFD-2B4A-40B5-B952-52002141C5DD}", "Idx", "Idx", true, true, param_fm_idx, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0, 16, 1, 2, ""), // todo default/max
    make_param_gui(section_fm, gui_edit_type::hslider, param_layout::vertical, { 0, 3 }, make_label_none())));
  fm_amount.gui.tabular = true;
  fm_amount.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  auto& fm_dly = result.params.emplace_back(make_param(
    make_topo_info("{277ED206-E225-46C9-BFBF-DC277C7F264A}", "Dly", "Dly", true, true, param_fm_dly, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_linear(0, 5, 0, 2, "Ms"), // todo default/max
    make_param_gui(section_fm, gui_edit_type::hslider, param_layout::vertical, { 0, 4 }, make_label_none())));
  fm_dly.gui.tabular = true;
  fm_dly.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

osc_matrix_engine::
osc_matrix_engine(int max_frame_count) : 
_am_modulator(this), _fm_modulator(this) 
{
  _context.am_modulator = &_am_modulator;
  _context.fm_modulator = &_fm_modulator;

  // for am we can return the unmodulated signal itself
  // but fm needs to return something that oscillator uses to adjust the phase
  // so in case no routes point to target Osc N we return a bunch of zeros
  _no_fm.resize(jarray<int, 2>(max_unison_voices + 1, jarray<int, 1>(2, max_frame_count)));
}

jarray<float, 3> const&
osc_matrix_am_modulator::modulate_am(
  plugin_block& block, int slot, 
  cv_matrix_mixdown const* cv_modulation)
{ return _engine->modulate_am(block, slot, cv_modulation); }

jarray<float, 3> const&
osc_matrix_fm_modulator::modulate_fm(
  plugin_block& block, int slot, 
  cv_matrix_mixdown const* cv_modulation)
{ return _engine->modulate_fm(block, slot, cv_modulation); }

void
osc_matrix_engine::process(plugin_block& block)
{
  // need to capture own audio here because when we start 
  // modulating "own" does not refer to us but to the caller
  *block.state.own_context = &_context;
  _own_audio = &block.state.own_audio;
}

// This returns the final output signal i.e. all modulators applied to carrier.
jarray<float, 3> const& 
osc_matrix_engine::modulate_am(
  plugin_block& block, int slot, cv_matrix_mixdown const* cv_modulation)
{
  // allow custom data for graphs
  if(cv_modulation == nullptr)
    cv_modulation = &get_cv_matrix_mixdown(block, false);

  // loop through the routes
  // the first match we encounter becomes the modulation result
  jarray<float, 3>* modulated = nullptr;
  jarray<float, 3> const& target_audio = block.voice->all_audio[module_osc][slot][0];
  auto const& block_auto = block.state.all_block_automation[module_osc_matrix][0];

  for (int r = 0; r < route_count; r++)
  {
    if(block_auto[param_am_on][r].step() == 0) continue;
    int target_osc = block_auto[param_am_target][r].step();
    if(target_osc != slot) continue;

    int target_uni_voices = block.state.all_block_automation[module_osc][target_osc][osc_param_uni_voices][0].step();
    if (modulated == nullptr)
    {
      modulated = &(*_own_audio)[r];
      for (int v = 0; v < target_uni_voices; v++)
      {
        // base value is the unmodulated target (carrier)
        // then keep multiplying by modulators
        target_audio[v + 1][0].copy_to(block.start_frame, block.end_frame, (*modulated)[v + 1][0]);
        target_audio[v + 1][1].copy_to(block.start_frame, block.end_frame, (*modulated)[v + 1][1]);
      }
    }

    // apply modulation on per unison voice level
    // mapping both source count and target count to [0, 1]
    // then linear interpolate. this allows modulation
    // between oscillators with unequal unison voice count
    int source_osc = block_auto[param_am_source][r].step();
    auto const& source_audio = block.module_audio(module_osc, source_osc);
    auto const& amt_curve = *(*cv_modulation)[module_osc_matrix][0][param_am_amt][r];
    auto const& ring_curve = *(*cv_modulation)[module_osc_matrix][0][param_am_ring][r];
    int source_uni_voices = block.state.all_block_automation[module_osc][source_osc][osc_param_uni_voices][0].step();

    for(int v = 0; v < target_uni_voices; v++)
    {
      float target_voice_pos = target_uni_voices == 1? 0.5f: v / (target_uni_voices - 1.0f);
      float source_voice = target_voice_pos * (source_uni_voices - 1);
      int source_voice_0 = (int)source_voice;
      int source_voice_1 = source_voice_0 + 1;
      float source_voice_pos = source_voice - (int)source_voice;
      if(source_voice_1 == source_uni_voices) source_voice_1--;

      for(int c = 0; c < 2; c++)
        for(int f = block.start_frame; f < block.end_frame; f++)
        {
          float audio = (*modulated)[v + 1][c][f];
          // do not assume [-1, 1] here as oscillator can go beyond that
          float rm0 = source_audio[0][source_voice_0 + 1][c][f];
          float rm1 = source_audio[0][source_voice_1 + 1][c][f];
          float rm = (1 - source_voice_pos) * rm0 + source_voice_pos * rm1;
          // "bipolar to unipolar" for [-inf, +inf]
          float am = (rm * 0.5f) + 0.5f;
          float mod = mix_signal(ring_curve[f], am, rm);
          (*modulated)[v + 1][c][f] = mix_signal(amt_curve[f], audio, mod * audio);
        }
    }
  }

  // default result is unmodulated (e.g., osc output itself)
  if(modulated != nullptr) return *modulated;
  return block.voice->all_audio[module_osc][slot][0];
}

// This returns stacked modulators but doesnt touch the carrier.
// Oscillator process() applies stacked modulation to the phase.
// TODO use the oversampled signal
// TODO do self-mod
// TODO use the feedback param
jarray<float, 3> const&
osc_matrix_engine::modulate_fm(
  plugin_block& block, int slot, cv_matrix_mixdown const* cv_modulation)
{
  // allow custom data for graphs
  if (cv_modulation == nullptr)
    cv_modulation = &get_cv_matrix_mixdown(block, false);

  // loop through the routes
  // the first match we encounter becomes the modulator result
  jarray<float, 3>* modulator = nullptr;
  auto const& block_auto = block.state.all_block_automation[module_osc_matrix][0];

  for (int r = 0; r < route_count; r++)
  {
    if (block_auto[param_fm_on][r].step() == 0) continue;
    int target_osc = block_auto[param_fm_target][r].step();
    if (target_osc != slot) continue;

    int target_uni_voices = block.state.all_block_automation[module_osc][target_osc][osc_param_uni_voices][0].step();
    if (modulator == nullptr)
    {
      modulator = &(*_own_audio)[r + route_count];
      for (int v = 0; v < target_uni_voices; v++)
      {
        // base value is zero (eg no phase modulation)
        // then keep adding modulators
        (*modulator)[v + 1][0].fill(0.0f);
        (*modulator)[v + 1][1].fill(0.0f);
      }
    }

    // apply modulation on per unison voice level
    // mapping both source count and target count to [0, 1]
    // then linear interpolate. this allows modulation
    // between oscillators with unequal unison voice count
    int source_osc = block_auto[param_fm_source][r].step();
    auto const& source_audio = block.module_audio(module_osc, source_osc);
    auto const& idx_curve = *(*cv_modulation)[module_osc_matrix][0][param_fm_idx][r];
    int source_uni_voices = block.state.all_block_automation[module_osc][source_osc][osc_param_uni_voices][0].step();

    for (int v = 0; v < target_uni_voices; v++)
    {
      float target_voice_pos = target_uni_voices == 1 ? 0.5f : v / (target_uni_voices - 1.0f);
      float source_voice = target_voice_pos * (source_uni_voices - 1);
      int source_voice_0 = (int)source_voice;
      int source_voice_1 = source_voice_0 + 1;
      float source_voice_pos = source_voice - (int)source_voice;
      if (source_voice_1 == source_uni_voices) source_voice_1--;

      for (int c = 0; c < 2; c++)
        for (int f = block.start_frame; f < block.end_frame; f++)
        {
          // do not assume [-1, 1] here as oscillator can go beyond that
          float mod0 = source_audio[0][source_voice_0 + 1][c][f];
          float mod1 = source_audio[0][source_voice_1 + 1][c][f];
          float mod = (1 - source_voice_pos) * mod0 + source_voice_pos * mod1;
          (*modulator)[v + 1][c][f] += idx_curve[f] * mod;
        }
    }
  }

  // default result is unmodulated (e.g., zero)
  if (modulator != nullptr) return *modulator;
  return _no_fm;
}

}
