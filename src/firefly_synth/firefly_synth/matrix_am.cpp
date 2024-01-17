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

enum { section_main };
enum { output_modulated };
enum { param_on, param_source, param_target, param_amt, param_ring };

static int const route_count = 20; // TODO decrease once we do FM
std::unique_ptr<graph_engine> make_osc_graph_engine(plugin_desc const* desc);
std::vector<graph_data> render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot);

class am_matrix_engine:
public module_engine { 
  am_matrix_modulator _modulator;
  jarray<float, 4>* _own_audio = {};
public:
  am_matrix_engine() : _modulator(this) {}
  PB_PREVENT_ACCIDENTAL_COPY(am_matrix_engine);

  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;
  jarray<float, 2> const& modulate(plugin_block& block, int slot, cv_matrix_mixdown const* cv_modulation);
};

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int max_osc = 0;
  std::vector<float> result_l;
  std::vector<float> result_r;
  for(int r = 0; r < route_count; r++)
    if(state.get_plain_at(module_am_matrix, 0, param_on, r).step() != 0)
      max_osc = std::max(max_osc, state.get_plain_at(module_am_matrix, 0, param_target, r).step());
  auto graphs(render_osc_graphs(state, engine, max_osc));
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
make_audio_routing_am_params(plugin_state* state)
{
  audio_routing_audio_params result;
  result.off_value = 0;
  result.on_param = param_on;
  result.source_param = param_source;
  result.target_param = param_target;
  result.matrix_module = module_am_matrix;
  result.sources = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }).mappings;
  result.targets = make_audio_matrix({ &state->desc().plugin->modules[module_osc] }).mappings;
  return result;
}

module_topo 
am_matrix_topo(int section, gui_colors const& colors, gui_position const& pos, plugin_topo const* plugin)
{
  auto am_matrix = make_audio_matrix({ &plugin->modules[module_osc] });

  module_topo result(make_module(
    make_topo_info("{8024F4DC-5BFC-4C3D-8E3E-C9D706787362}", "Osc AM", "AM", true, true, module_am_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{1DABDF9D-E777-44FF-9720-3B09AAF07C6D}", "Modulated", output_modulated, route_count)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.tabbed_name = result.info.tag.name;
  result.engine_factory = [](auto const& topo, int, int) { return std::make_unique<am_matrix_engine>(); };
  result.gui.menu_handler_factory = [](plugin_state* state) { return std::make_unique<tidy_matrix_menu_handler>(
    state, param_on, 0, std::vector<int>({ param_target, param_source })); };

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A48C0675-C020-4D05-A384-EF2B8CA8A066}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 1, 1, -35, -35 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "On", param_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  on.gui.tabular = true;
  on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, route_count, 1); };

  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{1D8F3294-2463-470D-853B-561E8228467A}", "Source", param_source, route_count),
    make_param_dsp_voice(param_automate::none), make_domain_item(am_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.tabular = true;
  source.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  source.gui.item_enabled.bind_param({ module_am_matrix, 0, param_target, gui_item_binding::match_param_slot },
    [am = am_matrix.mappings](int other, int self) { return am[self].slot <= am[other].slot; });

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{1AF0E66A-ADB5-40F4-A4E1-9F31941171E2}", "Target", param_target, route_count),
    make_param_dsp_voice(param_automate::none), make_domain_item(am_matrix.items, "Osc 1"),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.tabular = true;
  target.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  target.gui.item_enabled.bind_param({ module_am_matrix, 0, param_source, gui_item_binding::match_param_slot },
    [am = am_matrix.mappings](int other, int self) { return am[other].slot <= am[self].slot; });

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{A1A7298E-542D-4C2F-9B26-C1AF7213D095}", "Amt", param_amt, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  amount.gui.tabular = true;
  amount.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& ring = result.params.emplace_back(make_param(
    make_topo_info("{3DF51ADC-9882-4F95-AF4E-5208EB14E645}", "Ring", param_ring, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 }, make_label_none())));
  ring.gui.tabular = true;
  ring.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

jarray<float, 2> const&
am_matrix_modulator::modulate(plugin_block& block, int slot, cv_matrix_mixdown const* cv_modulation)
{ return _engine->modulate(block, slot, cv_modulation); }

void
am_matrix_engine::process(plugin_block& block)
{
  // need to capture own audio here because when we start 
  // modulating "own" does not refer to us but to the caller
  *block.state.own_context = &_modulator;
  _own_audio = &block.state.own_audio;
}

jarray<float, 2> const& 
am_matrix_engine::modulate(plugin_block& block, int slot, cv_matrix_mixdown const* cv_modulation)
{
  // allow custom data for graphs
  if(cv_modulation == nullptr)
    cv_modulation = &get_cv_matrix_mixdown(block, false);

  // loop through the routes
  // the first match we encounter becomes the modulation result
  jarray<float, 2>* modulated = nullptr;
  jarray<float, 2> const& target_audio = block.voice->all_audio[module_osc][slot][0][0];
  auto const& block_auto = block.state.all_block_automation[module_am_matrix][0];
  for (int r = 0; r < route_count; r++)
  {
    if(block_auto[param_on][r].step() == 0) continue;
    int target_osc = block_auto[param_target][r].step();
    if(target_osc != slot) continue;
    if (modulated == nullptr)
    {
      modulated = &(*_own_audio)[output_modulated][r];
      target_audio[0].copy_to(block.start_frame, block.end_frame, (*modulated)[0]);
      target_audio[1].copy_to(block.start_frame, block.end_frame, (*modulated)[1]);
    }

    // apply modulation
    int source_osc = block_auto[param_source][r].step();
    auto const& source_audio = block.module_audio(module_osc, source_osc);
    auto const& amt_curve = *(*cv_modulation)[module_am_matrix][0][param_amt][r];
    auto const& ring_curve = *(*cv_modulation)[module_am_matrix][0][param_ring][r];
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
      {
        float audio = (*modulated)[c][f];
        float rm = check_bipolar(source_audio[0][0][c][f]);
        float am = bipolar_to_unipolar(rm);
        float mod = mix_signal(ring_curve[f], am, rm);
        (*modulated)[c][f] = mix_signal(amt_curve[f], audio, mod * audio);
      }
  }

  // default result is unmodulated (e.g., osc output itself)
  if(modulated != nullptr) return *modulated;
  return block.voice->all_audio[module_osc][slot][0][0];
}

}
