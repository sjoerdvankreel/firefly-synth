#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth { 

enum { section_am, section_fm };
enum { scratch_fm_idx, scratch_count };

// it appears the "magical" through zero mode is really just about 
// modulation by an unipolar vs bipolar (e.g. through-zero) signal?
// bipolar mod has the nice property that the carrier's phase can
// travel "backwards" so produces a distinct sound from unipolar mod
// https://ristoid.net/modular/fm_variants.html
enum { fm_mode_bipolar, fm_mode_unipolar };

enum { 
  param_am_on, param_am_source, param_am_target, param_am_amt, param_am_ring,
  param_fm_on, param_fm_source, param_fm_target, param_fm_mode, param_fm_idx
};

static int const route_count = 8;
extern int const osc_param_type;
extern int const osc_param_uni_voices;
extern int const voice_in_param_oversmp;

std::unique_ptr<graph_engine> make_osc_graph_engine(plugin_desc const* desc);
std::vector<graph_data> render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot, bool for_osc_osc_matrix);

static std::vector<list_item>
fm_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{B5CD2CE9-89C0-4E15-87E9-D8EF4D399EE6}", "Bipolar"); // thru-zero
  result.emplace_back("{0E688960-E59A-4E78-8812-6BADDAF881B8}", "Unipolar");
  return result;
}

class osc_osc_matrix_engine:
public module_engine { 
  jarray<float, 2> _no_fm = {};
  jarray<float, 3> _fm_modsig = {};
  osc_osc_matrix_context _context = {};
  jarray<float, 4>* _own_audio = {};
  jarray<float, 2>* _own_scratch = {};
  osc_osc_matrix_am_modulator _am_modulator;
  osc_osc_matrix_fm_modulator _fm_modulator;
public:
  osc_osc_matrix_engine(int max_frame_count);
  PB_PREVENT_ACCIDENTAL_COPY(osc_osc_matrix_engine);

  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;

  jarray<float, 3> const& modulate_am(
    plugin_block& block, int slot, 
    cv_audio_matrix_mixdown const* cv_modulation);

  template <bool Graph>
  jarray<float, 2> const& modulate_fm(
    plugin_block& block, int slot,
    cv_audio_matrix_mixdown const* cv_modulation);
};

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int max_osc = 0;
  std::vector<float> result_l;
  std::vector<float> result_r;
  for(int r = 0; r < route_count; r++)
    if(state.get_plain_at(module_osc_osc_matrix, 0, param_am_on, r).step() != 0)
      max_osc = std::max(max_osc, state.get_plain_at(module_osc_osc_matrix, 0, param_am_target, r).step());
  for (int r = 0; r < route_count; r++)
    if (state.get_plain_at(module_osc_osc_matrix, 0, param_fm_on, r).step() != 0)
      max_osc = std::max(max_osc, state.get_plain_at(module_osc_osc_matrix, 0, param_fm_target, r).step());
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
  result.matrix_module = module_osc_osc_matrix;
  result.matrix_on_params = { param_am_on, param_fm_on };
  result.matrix_source_params = { param_am_source, param_fm_source };
  result.matrix_target_params = { param_am_target, param_fm_target };
  result.sources = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }, 0).mappings;
  result.targets = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }, 0).mappings;
  return result;
}

module_topo 
osc_osc_matrix_topo(int section, gui_colors const& colors, gui_position const& pos, plugin_topo const* plugin)
{
  std::vector<module_dsp_output> outputs;
  auto osc_matrix = make_audio_matrix({ &plugin->modules[module_osc] }, 0);

  // scratch state for AM
  // for FM we use oversampled mono series
  for(int r = 0; r < route_count; r++)
    outputs.push_back(make_module_dsp_output(false, make_topo_info_basic(
      "{1DABDF9D-E777-44FF-9720-3B09AAF07C6D}-" + std::to_string(r), "AM", r, max_unison_voices + 1)));

  module_topo result(make_module(
    make_topo_info("{8024F4DC-5BFC-4C3D-8E3E-C9D706787362}", true, "Osc Mod", "Osc", "Osc", module_osc_osc_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::audio, scratch_count, outputs),
    make_module_gui(section, colors, pos, { 2, 1 })));
  result.info.description = "Oscillator routing matrices that allow for Osc-to-Osc AM, RM and FM.";

  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.tabbed_name = result.info.tag.menu_display_name;
  result.engine_factory = [](auto const& topo, int sr, int max_frame_count) { return std::make_unique<osc_osc_matrix_engine>(max_frame_count); };
  result.gui.menu_handler_factory = [](plugin_state* state) { return std::make_unique<tidy_matrix_menu_handler>(
    state, 2, param_am_on, 0, std::vector<std::vector<int>>({{ param_am_target, param_am_source }, { param_fm_target, param_fm_source } })); };

  auto& am = result.sections.emplace_back(make_param_section(section_am,
    make_topo_tag_basic("{A48C0675-C020-4D05-A384-EF2B8CA8A066}", "AM"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 2, 2, 3, 3 } })));
  am.gui.scroll_mode = gui_scroll_mode::vertical;  
  auto& am_on = result.params.emplace_back(make_param(
    make_topo_info_basic("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "AM", param_am_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_am, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  am_on.gui.tabular = true;
  am_on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, 2, 0, route_count, 1); };
  am_on.info.description = "Toggles AM routing on/off.";
  auto& am_source = result.params.emplace_back(make_param(
    make_topo_info_basic("{1D8F3294-2463-470D-853B-561E8228467A}", "Source", param_am_source, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, ""),
    make_param_gui(section_am, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  am_source.gui.tabular = true;
  am_source.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_source.gui.item_enabled.bind_param({ module_osc_osc_matrix, 0, param_am_target, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[self].slot <= osc[other].slot; });
  am_source.info.description = "Selects AM routing source. Note that you can only route 'upwards', so not Osc2->Osc1. However self-modulation is possible.";
  auto& am_target = result.params.emplace_back(make_param(
    make_topo_info_basic("{1AF0E66A-ADB5-40F4-A4E1-9F31941171E2}", "Target", param_am_target, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, "Osc 2"),
    make_param_gui(section_am, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  am_target.gui.tabular = true;
  am_target.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_target.gui.item_enabled.bind_param({ module_osc_osc_matrix, 0, param_am_source, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) { 
      return osc[other].slot <= osc[self].slot; });
  am_target.info.description = "Selects AM routing target.";
  auto& am_amount = result.params.emplace_back(make_param(
    make_topo_info_basic("{A1A7298E-542D-4C2F-9B26-C1AF7213D095}", "Mix", param_am_amt, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui(section_am, gui_edit_type::hslider, param_layout::vertical, { 0, 3 }, make_label_none())));
  am_amount.gui.tabular = true;
  am_amount.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_amount.info.description = "Dry/wet control between unmodulated and modulated signal.";
  auto& am_ring = result.params.emplace_back(make_param(
    make_topo_info_basic("{3DF51ADC-9882-4F95-AF4E-5208EB14E645}", "Ring", param_am_ring, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_am, gui_edit_type::hslider, param_layout::vertical, { 0, 4 }, make_label_none())));
  am_ring.gui.tabular = true;
  am_ring.gui.bindings.enabled.bind_params({ param_am_on }, [](auto const& vs) { return vs[0] != 0; });
  am_ring.info.description = "Dry/wet control between amplitude-modulated and ring-modulated signal.";

  auto& fm = result.sections.emplace_back(make_param_section(section_fm,
    make_topo_tag_basic("{1B39A828-3429-4245-BF07-551C17A78341}", "FM"),
    make_param_section_gui({ 1, 0 }, { { 1 }, { -25, 2, 2, 2, 4 } })));
  fm.gui.scroll_mode = gui_scroll_mode::vertical;
  auto& fm_on = result.params.emplace_back(make_param(
    make_topo_info_basic("{02112C80-D1E9-409E-A9FB-6DCA34F5CABA}", "FM", param_fm_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_fm, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  fm_on.gui.tabular = true;
  fm_on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, 2, 1, route_count, 1); };
  fm_on.info.description = "Toggles FM routing on/off.";
  auto& fm_source = result.params.emplace_back(make_param(
    make_topo_info_basic("{61E9C704-E704-4669-9DC3-D3AA9FD6A952}", "Source", param_fm_source, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, ""),
    make_param_gui(section_fm, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  fm_source.gui.tabular = true;
  fm_source.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_source.gui.item_enabled.bind_param({ module_osc_osc_matrix, 0, param_fm_target, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[self].slot < osc[other].slot; });
  fm_source.info.description = std::string("Selects FM routing source. Note that you can only route 'upwards', so not Osc2->Osc1. ") + 
    "Self-modulation is not possible (AKA, feedback-FM not implemented).";
  auto& fm_target = result.params.emplace_back(make_param(
    make_topo_info_basic("{DBDD28D6-46B9-4F9A-9682-66E68A261B87}", "Target", param_fm_target, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(osc_matrix.items, "Osc 2"),
    make_param_gui(section_fm, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  fm_target.gui.tabular = true;
  fm_target.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_target.gui.item_enabled.bind_param({ module_osc_osc_matrix, 0, param_fm_source, gui_item_binding::match_param_slot },
    [osc = osc_matrix.mappings](int other, int self) {
      return osc[other].slot < osc[self].slot; });
  fm_target.info.description = "Selects FM routing target.";
  auto& fm_mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{277ED206-E225-46C9-BFBF-DC277C7F264A}", "Mode", param_fm_mode, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_item(fm_mode_items(), ""),
    make_param_gui(section_fm, gui_edit_type::list, param_layout::vertical, { 0, 3 }, make_label_none())));
  fm_mode.gui.tabular = true;
  fm_mode.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_mode.info.description = std::string("Selects unipolar/bipolar mode. ") + 
    "Bipolar causes the target Osc's phase to travel both forward and backward and is apparently referred to as through-zero FM.";
  auto& fm_amount = result.params.emplace_back(make_param(
    make_topo_info_basic("{444B0AFD-2B4A-40B5-B952-52002141C5DD}", "Index", param_fm_idx, route_count), // TODO full-name FM-index for automation. Probably others for the matrices.
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(0, 1, 0.01, 0.05, 4, ""),
    make_param_gui(section_fm, gui_edit_type::hslider, param_layout::vertical, { 0, 4 }, make_label_none())));
  fm_amount.gui.tabular = true;
  fm_amount.gui.bindings.enabled.bind_params({ param_fm_on }, [](auto const& vs) { return vs[0] != 0; });
  fm_amount.info.description = std::string("Modulation index. This is really just a multiplier for the source signal. ") + 
    "Less index is less phase adjustment on the target signal. I did not implement automatic scaling with pitch, " + 
    "but this is a modulatable parameter which you can couple with the Note Key modulation source.";

  return result;
}

osc_osc_matrix_engine::
osc_osc_matrix_engine(int max_frame_count) :
_am_modulator(this), _fm_modulator(this) 
{
  _context.am_modulator = &_am_modulator;
  _context.fm_modulator = &_fm_modulator;

  // for am we can return the unmodulated signal itself
  // but fm needs to return something that oscillator uses to adjust the phase
  // so in case no routes point to target Osc N we return a bunch of zeros
  _no_fm.resize(jarray<int, 1>(max_unison_voices + 1, max_frame_count * (1 << max_oversampler_stages)));
  _fm_modsig.resize(jarray<int, 2>(route_count, jarray<int, 1>(max_unison_voices + 1, max_frame_count * (1 << max_oversampler_stages))));
}

// unison-channel-frame
jarray<float, 3> const&
osc_osc_matrix_am_modulator::modulate_am(
  plugin_block& block, int slot, 
  cv_audio_matrix_mixdown const* cv_modulation)
{ return _engine->modulate_am(block, slot, cv_modulation); }

// unison-(frame*oversmp)
template <bool Graph>
jarray<float, 2> const&
osc_osc_matrix_fm_modulator::modulate_fm(
  plugin_block& block, int slot, 
  cv_audio_matrix_mixdown const* cv_modulation)
{ return _engine->modulate_fm<Graph>(block, slot, cv_modulation); }

// need explicit instantiation here
template
jarray<float, 2> const&
osc_osc_matrix_fm_modulator::modulate_fm<false>(plugin_block& block, int slot, cv_audio_matrix_mixdown const* cv_modulation);
template
jarray<float, 2> const&
osc_osc_matrix_fm_modulator::modulate_fm<true>(plugin_block& block, int slot, cv_audio_matrix_mixdown const* cv_modulation);

void
osc_osc_matrix_engine::process(plugin_block& block)
{
  // need to capture stuff here because when we start 
  // modulating "own" does not refer to us but to the caller
  *block.state.own_context = &_context;
  _own_audio = &block.state.own_audio;
  _own_scratch = &block.state.own_scratch;
}

// This returns the final output signal i.e. all modulators applied to carrier.
jarray<float, 3> const& 
osc_osc_matrix_engine::modulate_am(
  plugin_block& block, int slot, cv_audio_matrix_mixdown const* cv_modulation)
{
  // allow custom data for graphs
  if(cv_modulation == nullptr)
    cv_modulation = &get_cv_audio_matrix_mixdown(block, false);

  // loop through the routes
  // the first match we encounter becomes the modulation result
  jarray<float, 3>* modulated = nullptr;
  jarray<float, 3> const& target_audio = block.voice->all_audio[module_osc][slot][0];
  auto const& block_auto = block.state.all_block_automation[module_osc_osc_matrix][0];

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
    auto const& amt_curve = *(*cv_modulation)[module_osc_osc_matrix][0][param_am_amt][r];
    auto const& ring_curve = *(*cv_modulation)[module_osc_osc_matrix][0][param_am_ring][r];
    int source_uni_voices = block.state.all_block_automation[module_osc][source_osc][osc_param_uni_voices][0].step();

    for(int v = 0; v < target_uni_voices; v++)
    {
      // lerp unison voices
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
template <bool Graph>
jarray<float, 2> const&
osc_osc_matrix_engine::modulate_fm(
  plugin_block& block, int slot, cv_audio_matrix_mixdown const* cv_modulation)
{
  // allow custom data for graphs
  if (cv_modulation == nullptr)
    cv_modulation = &get_cv_audio_matrix_mixdown(block, false);

  // loop through the routes
  // the first match we encounter becomes the modulator result
  jarray<float, 2>* modulator = nullptr;
  auto const& block_auto = block.state.all_block_automation[module_osc_osc_matrix][0];

  for (int r = 0; r < route_count; r++)
  {
    if (block_auto[param_fm_on][r].step() == 0) continue;
    int target_osc = block_auto[param_fm_target][r].step();
    if (target_osc != slot) continue;

    int target_uni_voices = block.state.all_block_automation[module_osc][target_osc][osc_param_uni_voices][0].step();
    if (modulator == nullptr)
    {
      // modulator = uni voices times oversmp factor
      // no channel dimension here
      modulator = &_fm_modsig[r];
      for (int v = 0; v < target_uni_voices; v++)
      {
        // base value is zero (eg no phase modulation)
        // then keep adding modulators
        (*modulator)[v + 1].fill(0.0f);
      }
    }

    // through zero stuff
    int route_mode = block_auto[param_fm_mode][r].step();
    assert(route_mode == fm_mode_bipolar || route_mode == fm_mode_unipolar);
    float mode_add;
    float mode_mul;
    if (route_mode == fm_mode_bipolar)
    {
      mode_add = 0;
      mode_mul = 1;
    }
    else
    {
      mode_add = 0.5f;
      mode_mul = 0.5f;
    }

    // apply modulation on per unison voice level
    // mapping both source count and target count to [0, 1]
    // then linear interpolate. this allows modulation
    // between oscillators with unequal unison voice count
    int source_osc = block_auto[param_fm_source][r].step();
    auto const& idx_curve_plain = *(*cv_modulation)[module_osc_osc_matrix][0][param_fm_idx][r];
    auto& idx_curve = (*_own_scratch)[scratch_fm_idx];
    normalized_to_raw_into_fast<domain_type::log>(block, module_osc_osc_matrix, param_fm_idx, idx_curve_plain, idx_curve);
    int source_uni_voices = block.state.all_block_automation[module_osc][source_osc][osc_param_uni_voices][0].step();
    int oversmp_stages = block.state.all_block_automation[module_voice_in][0][voice_in_param_oversmp][0].step();
    int oversmp_factor = 1 << oversmp_stages;
    
    // Oscs are NOT oversampled in this case.
    if (Graph) 
    {
      oversmp_factor = 1;
      oversmp_stages = 0;
    }

    // oscillator provides us with the upsampled buffers to we can do oversampled FM
    void* source_context_ptr = block.module_context(module_osc, source_osc);
    auto source_context = static_cast<oscillator_context*>(source_context_ptr);
    auto source_audio = source_context->oversampled_lanes_channels_ptrs[oversmp_stages];

    // in case the source osc is off, the oversampled signal is *not* generated
    // and it will contain garbage from the last time the osc was active
    // so account for that (really it's just weird config by the user, but still
    // should not produce garbage audio)
    bool source_off = block.state.all_block_automation[module_osc][source_osc][osc_param_type][0].step() == 0;
    float source_multiplier = source_off? 0: 1;

    for (int v = 0; v < target_uni_voices; v++)
    {
      // lerp unison voices
      float target_voice_pos = target_uni_voices == 1 ? 0.5f : v / (target_uni_voices - 1.0f);
      float source_voice = target_voice_pos * (source_uni_voices - 1);
      int source_voice_0 = (int)source_voice;
      int source_voice_1 = source_voice_0 + 1;
      float source_voice_pos = source_voice - (int)source_voice;
      if (source_voice_1 == source_uni_voices) source_voice_1--;

      // note we work on the oversmp data from 0 to (end_frame - start_frame) * oversmp_factor
      for (int f = 0; f < (block.end_frame - block.start_frame) * oversmp_factor; f++)
      {
        // do not assume [-1, 1] here as oscillator can go beyond that
        // what to do with the stereo signal ? 
        // the oscs output a 2 channel signal but we only
        // need one. just take the average for now.
        // another option would be to have the osc output
        // an additional mono signal but i doubt its worth the bookkeeping
        
        // also the bookkeeping is already complicated here:
        // source_audio[(unison_voice + 1) * stero_channels + stereo_channel)
        // where + 1 is because voice 0 is the mixdown of all unison voices
        float mod0_l = source_audio[(source_voice_0 + 1) * 2 + 0][f];
        float mod0_r = source_audio[(source_voice_0 + 1) * 2 + 1][f];
        float mod1_l = source_audio[(source_voice_1 + 1) * 2 + 0][f];
        float mod1_r = source_audio[(source_voice_1 + 1) * 2 + 1][f];
        float mod0 = (mod0_l + mod0_r) * 0.5f;
        float mod1 = (mod1_l + mod1_r) * 0.5f;
        float mod = (1 - source_voice_pos) * mod0 + source_voice_pos * mod1;

        // in this case the oversampled pointers are not published
        // yet by the oscillators. since we dont oversample anyway
        // for graphs, just reach back into the original osc-provided
        // audio buffers
        if constexpr (Graph)
        {
          mod0 = block.module_audio(module_osc, source_osc)[0][source_voice_0 + 1][0][f];
          mod1 = block.module_audio(module_osc, source_osc)[0][source_voice_1 + 1][0][f];
          mod = (1 - source_voice_pos) * mod0 + source_voice_pos * mod1;
        }

        // through zero + account for inactive source osc
        mod = ((mode_mul * mod) + mode_add) * source_multiplier;

        // oversampler is from 0 to (end_frame - start_frame) * oversmp_factor
        // all the not-oversampled stuff requires from start_frame to end_frame
        // so mind the bookkeeping
        int mod_index = block.start_frame + f / oversmp_factor;

        // fm modulation is oversampled so "f"
        // automation is NOT oversampled so "mod_index"
        (*modulator)[v + 1][f] += idx_curve[mod_index] * mod;
      }
    }
  }

  // default result is unmodulated (e.g., zero)
  if (modulator != nullptr) return *modulator;
  return _no_fm;
}

}
