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

static int constexpr vcv_route_count = 20;
static int constexpr gcv_route_count = 16;
static int constexpr max_cv_route_count = std::max(vcv_route_count, gcv_route_count);

static int constexpr vaudio_route_count = 30;
static int constexpr gaudio_route_count = 20;
static int constexpr max_audio_route_count = std::max(vaudio_route_count, gaudio_route_count);

static int constexpr max_any_route_count = std::max(max_cv_route_count, max_audio_route_count);

enum { section_main };
enum { scratch_transform_source, scratch_offset, scratch_scale, scratch_count };
enum { param_type, param_source, param_target, param_offset, param_scale, param_min, param_max };
enum { type_off, type_mul_abs, type_mul_rel, type_mul_stk, type_add_abs, type_add_rel, type_add_stk, type_ab_abs, type_ab_rel, type_ab_stk };

// for every param that gets modulated by the cv matrix,
// we output the final value per block as parameter modulation output
struct mod_output_usage
{
  bool in_use;
  int param_global;
  int module_global;
  jarray<float, 1>* modulated_curve_ptr;
};

static int
route_count_from_module(int module)
{
  switch (module)
  {
  case module_vcv_cv_matrix: return vcv_route_count;
  case module_gcv_cv_matrix: return gcv_route_count;
  case module_vcv_audio_matrix: return vaudio_route_count;
  case module_gcv_audio_matrix: return gaudio_route_count;
  default: assert(false); return -1;
  }
}

static int
route_count_from_matrix_type(bool cv, bool global)
{
  if(cv) return global? gcv_route_count: vcv_route_count;
  return global? gaudio_route_count: vaudio_route_count;
}

static int
module_from_matrix_type(bool cv, bool global)
{
  if(cv) return global? module_gcv_cv_matrix: module_vcv_cv_matrix;
  return global? module_gcv_audio_matrix: module_vcv_audio_matrix;
}

extern float
lfo_frequency_from_state(
  plugin_state const& state, int module_index, int module_slot, int bpm);
extern void
env_plot_length_seconds(
  plugin_state const& state, int slot, float& dly, float& att, float& hld, float& dcy, float& rls, float& flt);

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{7CE8B8A1-0711-4BDE-BDFF-0F97BF16EB57}", "Off");
  result.emplace_back("{C185C0A7-AE6A-4ADE-8171-119A96C24233}", "Mul Abs", "Multiply Absolute");
  result.emplace_back("{51FBF610-2039-448E-96B0-3C5FDED5DC7C}", "Mul Rel", "Multiply Relative To Param");
  result.emplace_back("{D9AEAC34-8E75-4EFD-91D3-6F9058226816}", "Mul Stk", "Multiply Relative To Stacked Modulation");
  result.emplace_back("{000C0860-B191-4554-9249-85846B1AFFD1}", "Add Abs", "Add Absolute");
  result.emplace_back("{169406D2-E86F-4275-A49F-59ED67CD7661}", "Add Rel", "Add Relative To Param");
  result.emplace_back("{621467B6-CFB7-4801-9DF4-6F9A200AD098}", "Add Stk", "Add Relative To Stacked Modulation");
  result.emplace_back("{23FB17DA-B98B-49FF-8D46-4E5FE7F486D6}", "AB Abs", "Bipolar Add Absolute");
  result.emplace_back("{6708DDD1-14EA-4E1D-8A1F-E4FFE76A87F0}", "AB Rel", "Bipolar Add Relative To Param");
  result.emplace_back("{1CCAB37F-0AA7-4A77-8C4C-28838970665B}", "AB Stk", "Bipolar Add Relative To Stacked Modulation");
  return result;
}

// shared cv->cv and cv->audio modulation
class cv_matrix_engine_base :
public module_engine {
protected:

  bool const _cv;
  bool const _global;
  cv_matrix_mixdown _mixdown = {};
  jarray<int, 4> _modulation_indices = {};
  std::vector<param_topo_mapping> const _targets;
  std::vector<module_output_mapping> const _sources;

  // need to remember this for access during the cv->cv mix() call
  jarray<float, 3>* _own_cv = {};
  jarray<float, 2>* _own_scratch = {};
  jarray<float, 3> const* _own_accurate_automation = {};
  jarray<plain_value, 2> const* _own_block_automation = {};

  cv_matrix_engine_base(
    bool cv, bool global, plugin_topo const& topo,
    std::vector<module_output_mapping> const& sources,
    std::vector<param_topo_mapping> const& targets);

  // module/slot:-1 is all (for cv->audio)
  // or specific module (for cv->cv)
  void perform_mixdown(plugin_block& block, int module, int slot);

public:
  void reset_audio(
    plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
};

// mixes down into a single cv (entire module) on demand
class cv_cv_matrix_engine :
public cv_matrix_engine_base {
  cv_cv_matrix_mixer _mixer;

public:
  cv_cv_matrix_engine(
    bool global, plugin_topo const& topo,
    std::vector<module_output_mapping> const& sources,
    std::vector<param_topo_mapping> const& targets):
  cv_matrix_engine_base(true, global, topo, sources, targets), _mixer(this) {}

  PB_PREVENT_ACCIDENTAL_COPY(cv_cv_matrix_engine);
  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  cv_cv_matrix_mixdown const& mix(plugin_block& block, int module, int slot);
};

// mixes down all cv to audio targets at once
class cv_audio_matrix_engine:
public cv_matrix_engine_base {
public:
  cv_audio_matrix_engine(
    bool global, plugin_topo const& topo, 
    std::vector<module_output_mapping> const& sources,
    std::vector<param_topo_mapping> const& targets):
  cv_matrix_engine_base(false, global, topo, sources, targets) {}

  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_audio_matrix_engine);
};

static void
init_audio_voice_default(plugin_state& state)
{
  state.set_text_at(module_vcv_audio_matrix, 0, param_type, 0, "Add Abs");
  state.set_text_at(module_vcv_audio_matrix, 0, param_source, 0, "Env 2");
  state.set_text_at(module_vcv_audio_matrix, 0, param_target, 0, "VFX 1 SVF Freq");
  state.set_text_at(module_vcv_audio_matrix, 0, param_type, 1, "AB Abs");
  state.set_text_at(module_vcv_audio_matrix, 0, param_min, 1, "35");
  state.set_text_at(module_vcv_audio_matrix, 0, param_max, 1, "65");
  state.set_text_at(module_vcv_audio_matrix, 0, param_source, 1, "GLFO 2");
  state.set_text_at(module_vcv_audio_matrix, 0, param_target, 1, "VAudio Balance 1"); 
  state.set_text_at(module_vcv_audio_matrix, 0, param_type, 2, "AB Abs");
  state.set_text_at(module_vcv_audio_matrix, 0, param_source, 2, "Global PB");
  state.set_text_at(module_vcv_audio_matrix, 0, param_target, 2, "Voice Pitch Bend");
  state.set_text_at(module_vcv_audio_matrix, 0, param_type, 3, "Mul Abs");
  state.set_text_at(module_vcv_audio_matrix, 0, param_source, 3, "Note Velo");
  state.set_text_at(module_vcv_audio_matrix, 0, param_target, 3, "VOut Gain");
}

static void
init_audio_global_default(plugin_state& state)
{
  state.set_text_at(module_gcv_audio_matrix, 0, param_type, 0, "AB Abs");
  state.set_text_at(module_gcv_audio_matrix, 0, param_min, 0, "35");
  state.set_text_at(module_gcv_audio_matrix, 0, param_max, 0, "65");
  state.set_text_at(module_gcv_audio_matrix, 0, param_source, 0, "GLFO 1");
  state.set_text_at(module_gcv_audio_matrix, 0, param_target, 0, "GFX 1 SVF Freq");
  state.set_text_at(module_gcv_audio_matrix, 0, param_type, 1, "Add Abs");
  state.set_text_at(module_gcv_audio_matrix, 0, param_source, 1, "Global Mod");
  state.set_text_at(module_gcv_audio_matrix, 0, param_target, 1, "GFX 1 SVF Freq");
}

audio_routing_cv_params
make_audio_routing_cv_params(plugin_state* state, bool global)
{
  audio_routing_cv_params result; 
  result.off_value = type_off;
  result.on_param = param_type;
  result.target_param = param_target;
  result.matrix_module = global ? module_gcv_audio_matrix : module_vcv_audio_matrix;
  result.targets = make_cv_target_matrix(make_cv_audio_matrix_targets(state->desc().plugin, global)).mappings;
  return result;
}

std::unique_ptr<module_tab_menu_handler>
make_cv_routing_menu_handler(plugin_state* state)
{
  std::map<int, std::vector<param_topo_mapping>> target_matrices;
  target_matrices[module_gcv_cv_matrix] = make_cv_target_matrix(make_cv_cv_matrix_targets(state->desc().plugin, true)).mappings;
  target_matrices[module_vcv_cv_matrix] = make_cv_target_matrix(make_cv_cv_matrix_targets(state->desc().plugin, false)).mappings;
  std::map<int, std::vector<module_output_mapping>> source_matrices;
  source_matrices[module_gcv_cv_matrix] = make_cv_source_matrix(state->desc().plugin, make_cv_matrix_sources(state->desc().plugin, true, false)).mappings;
  source_matrices[module_vcv_cv_matrix] = make_cv_source_matrix(state->desc().plugin, make_cv_matrix_sources(state->desc().plugin, false, false)).mappings;
  source_matrices[module_gcv_audio_matrix] = make_cv_source_matrix(state->desc().plugin, make_cv_matrix_sources(state->desc().plugin, true, false)).mappings;
  source_matrices[module_vcv_audio_matrix] = make_cv_source_matrix(state->desc().plugin, make_cv_matrix_sources(state->desc().plugin, false, false)).mappings;
  return std::make_unique<cv_routing_menu_handler>(state, param_type, type_off, param_source, param_target, source_matrices, target_matrices);
}

void
select_midi_active(
  plugin_state const& state, bool cv, bool global, int on_note_midi_start,
  std::vector<module_output_mapping> const& mappings, jarray<int, 3>& active)
{
  // PB and mod wheel are linked so must be always on
  active[module_midi][0][midi_source_pb] = 1;
  active[module_midi][0][midi_source_cc + 1] = 1;

  int module = module_from_matrix_type(cv, global);
  int route_count = route_count_from_matrix_type(cv, global);

  for (int r = 0; r < route_count; r++)
  {
    int type = state.get_plain_at(module, 0, param_type, r).step();
    if (type != type_off)
    {
      int source = state.get_plain_at(module, 0, param_source, r).step();
      auto const& mapping = mappings[source];
      if (mapping.module_index == module_midi)
      {
        if(mapping.output_index == midi_output_pb)
          active[module_midi][0][midi_source_pb] = 1;
        else if (mapping.output_index == midi_output_cp)
          active[module_midi][0][midi_source_cp] = 1;
        else if (mapping.output_index == midi_output_cc)
          active[module_midi][0][midi_source_cc + mapping.output_slot] = 1;
        else
          assert(false);
      }
      else if (mapping.module_index == module_voice_on_note)
      {
        // on note flattens the list of all global cv sources, with midi at end
        if(mapping.output_index == on_note_midi_start + midi_output_pb)
          active[module_midi][0][midi_source_pb] = 1;
        else if (mapping.output_index == on_note_midi_start + midi_output_cp)
          active[module_midi][0][midi_source_cp] = 1;
        else if (mapping.output_index >= on_note_midi_start + midi_output_cc && mapping.output_index < on_note_midi_start + midi_output_cc + 128)
          active[module_midi][0][midi_source_cc + mapping.output_index - on_note_midi_start - midi_output_cc] = 1;
      }
    }
  }
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = 400;
  result.midi_key = midi_middle_c;
  return result;
}

static std::unique_ptr<graph_engine>
make_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

// mapping to longest mod source for graph
static float
scale_to_longest_mod_source(
  plugin_state const& state, param_topo_mapping const& mapping,
  std::vector<module_output_mapping> const& sources, 
  float& max_dahd, float& max_dahdrf,
  int& module_index, int& module_slot)
{
  auto const& map = mapping;
  int route_count = route_count_from_module(map.module_index);

  // scale to longest env or lfo, prefer env
  float dahd = 0.01f;
  float dahdrf = 0.01f;
  float result = 0.01f;
  max_dahd = 0.01f;
  max_dahdrf = 0.01f;
  module_slot = -1;
  module_index = -1;

  float dly, att, hld, dcy, rls, flt;
  int ti = state.get_plain_at(map.module_index, map.module_slot, param_target, map.param_slot).step();

  // first check the envelopes, longest wins
  for (int r = 0; r < route_count; r++)
    if (state.get_plain_at(map.module_index, map.module_slot, param_type, r).step() != type_off)
      if (state.get_plain_at(map.module_index, map.module_slot, param_target, r).step() == ti)
      {
        int si = state.get_plain_at(map.module_index, map.module_slot, param_source, r).step();
        if (sources[si].module_index == module_env)
        {
          env_plot_length_seconds(state, sources[si].module_slot, dly, att, hld, dcy, rls, flt);
          dahd = dly + att + hld + dcy;
          dahdrf = dahd + rls + flt;
          if (dahdrf > max_dahdrf)
          {
            max_dahd = dahd;
            max_dahdrf = dahdrf;
            result = dahdrf;
            module_slot = sources[si].module_slot;
            module_index = sources[si].module_index;
          }
        }
      }

  // then check the lfos, longest wins
  if(module_index == -1)
    for (int r = 0; r < route_count; r++)
      if (state.get_plain_at(map.module_index, map.module_slot, param_type, r).step() != type_off)
        if (state.get_plain_at(map.module_index, map.module_slot, param_target, r).step() == ti)
        {
          int si = state.get_plain_at(map.module_index, map.module_slot, param_source, r).step();
          if (sources[si].module_index == module_glfo || sources[si].module_index == module_vlfo)
          {
            float freq = lfo_frequency_from_state(state, sources[si].module_index, sources[si].module_slot, 120);
            if (1 / freq > result)
            {
              result = 1 / freq;
              module_slot = sources[si].module_slot;
              module_index = sources[si].module_index;
            }
            result = std::max(result, 1 / freq);
          }
        }

  return result;
}

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param, 
  param_topo_mapping const& mapping, 
  std::vector<mod_out_custom_state> const& custom_outputs,
  std::vector<module_output_mapping> const& sources, 
  routing_matrix<param_topo_mapping> const& targets)
{
  auto const& map = mapping;
  int route_count = route_count_from_module(map.module_index);
  int type = state.get_plain_at(map.module_index, map.module_slot, param_type, map.param_slot).step();
  if(type == type_off) 
  {
    // try to always paint something
    for(int r = 0; r < route_count; r++)
      if(state.get_plain_at(map.module_index, map.module_slot, param_type, r).step() != type_off)
        return render_graph(state, engine, -1, { map.module_index, map.module_slot, map.param_index, r }, custom_outputs, sources, targets);
    return graph_data(graph_data_type::off, { state.desc().plugin->modules[mapping.module_index].info.tag.menu_display_name });
  }

  // scale to longest env or lfo
  float max_dahd;
  float max_dahdrf;
  int module_index;
  int module_slot;
  float max_total = scale_to_longest_mod_source(state, mapping, sources, max_dahd, max_dahdrf, module_index, module_slot);

  auto const params = make_graph_engine_params();
  int sample_rate = params.max_frame_count / max_total;
  int voice_release_at = max_dahd / max_dahdrf * params.max_frame_count;
  int ti = state.get_plain_at(map.module_index, map.module_slot, param_target, map.param_slot).step();

  // make dependent renderers know they are plotting for us
  std::vector<mod_out_custom_state> custom_cv_outputs(custom_outputs);
  custom_cv_outputs.push_back(modulation_output::make_mod_output_custom_state_int(
    -1, -1, custom_out_shared_render_for_cv_graph, -1).state.custom);

  engine->process_begin(&state, sample_rate, params.max_frame_count, voice_release_at);
  std::vector<int> relevant_modules({ module_gcv_cv_matrix, module_midi, module_global_in, module_glfo });
  if(map.module_index == module_vcv_audio_matrix || map.module_index == module_vcv_cv_matrix)
    relevant_modules.insert(relevant_modules.end(), { module_vcv_cv_matrix, module_voice_note, module_voice_on_note, module_arpeggiator, module_vlfo, module_env });
  for(int m = 0; m < relevant_modules.size(); m++)
    for(int mi = 0; mi < state.desc().plugin->modules[relevant_modules[m]].info.slot_count; mi++)
      engine->process_default(relevant_modules[m], mi, custom_cv_outputs, nullptr);
  auto* block = engine->process_default(map.module_index, map.module_slot, custom_cv_outputs, nullptr);
  engine->process_end();

  std::string partition = float_to_string(max_total, 1) + " Sec " + targets.items[ti].name;
  if(map.module_index == module_vcv_audio_matrix || map.module_index == module_gcv_audio_matrix)
  {
    // plotting cv->audio
    auto const& modulation = get_cv_audio_matrix_mixdown(*block, map.module_index == module_gcv_audio_matrix);
    jarray<float, 1> stacked = jarray<float, 1>(*targets.mappings[ti].value_at(modulation));
    return graph_data(stacked, false, 1.0f, false, { partition });
  } else
  {
    // plotting cv->cv
    auto const& target_map = targets.mappings[ti];
    auto& mixer = get_cv_cv_matrix_mixer(*block, map.module_index == module_gcv_cv_matrix);
    auto const& modulation = mixer.mix(*block, target_map.module_index, target_map.module_slot);
    jarray<float, 1> const* stacked = modulation[target_map.param_index][target_map.param_slot];
    return graph_data(*stacked, false, 1.0f, false, { partition });
  }
}

module_topo
cv_matrix_topo(
  plugin_topo const* topo, int section, 
  gui_position const& pos, bool cv, bool global, bool is_fx,
  std::vector<cv_source_entry> const& sources,
  std::vector<cv_source_entry> const& on_note_sources,
  std::vector<module_topo const*> const& targets)
{
  topo_info info;
  int on_note_midi_start = -1;
  auto source_matrix = make_cv_source_matrix(topo, sources);
  auto target_matrix = make_cv_target_matrix(targets);
  auto const vcv_info = make_topo_info_basic("{C21FFFB0-DD6E-46B9-89E9-01D88CE3DE46}", "VCV-CV", module_vcv_cv_matrix, 1);
  auto const gcv_info = make_topo_info_basic("{330B00F5-2298-4418-A0DC-521B30A8D72D}", "GCV-CV", module_gcv_cv_matrix, 1);
  auto const vaudio_info = make_topo_info_basic("{5F794E80-735C-43E8-B8EC-83910D118AF0}", "VCV-A", module_vcv_audio_matrix, 1);
  auto const gaudio_info = make_topo_info_basic("{DB22D4C1-EDA5-45F6-AE9B-183CA6F4C28D}", "GCV-A", module_gcv_audio_matrix, 1);

  if(cv) info = topo_info(global? gcv_info: vcv_info);
  else info = topo_info(global ? gaudio_info : vaudio_info);

  std::string matrix_type = cv? "CV-To-CV": "CV-To-Audio";
  module_stage stage = global ? module_stage::input : module_stage::voice;
  info.description = std::string(matrix_type + " routing matrix with min/max control and various stacking options ") +
    "that affect how source signals are combined in case they affect the same target.";

  int this_module = cv? module_gcv_cv_matrix: module_gcv_audio_matrix;
  if (!global)
  { 
    this_module = cv ? module_vcv_cv_matrix : module_vcv_audio_matrix;
    auto on_note_matrix(make_cv_source_matrix(topo, on_note_sources).mappings);
    for (int m = 0; m < on_note_matrix.size(); m++)
      if (on_note_matrix[m].module_index == module_midi) { on_note_midi_start = m; break; }
    assert(on_note_midi_start != -1);
    // these go first
    on_note_midi_start += on_voice_random_output_index + 1;
  }

  int route_count = route_count_from_matrix_type(cv, global);
  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, scratch_count, {
      make_module_dsp_output(false, -1, make_topo_info_basic("{3AEE42C9-691E-484F-B913-55EB05CFBB02}", "Output", 0, route_count)) }),
    make_module_gui(section, pos, { 1, 1 })));
  result.gui.render_automation_state = true; // don't want the graph source to be modulating the graph
  
  result.graph_engine_factory = make_graph_engine;
  if(!cv && !is_fx) result.default_initializer = global ? init_audio_global_default : init_audio_voice_default;
  result.graph_renderer = [sm = source_matrix.mappings, tm = target_matrix](
    auto const& state, auto* engine, int param, auto const& mapping, auto const& mods) {
      return render_graph(state, engine, param, mapping, mods, sm, tm);
    };

  // need these for continuous repaint
  result.dependent_custom_outputs_module_topo_indices = { module_midi, module_glfo };
  if (!global) result.dependent_custom_outputs_module_topo_indices = { 
    module_midi, module_glfo, module_arpeggiator, module_env, module_vlfo, module_voice_note, module_voice_on_note };
  result.gui.menu_handler_factory = [](plugin_state* state) {
    return std::make_unique<tidy_matrix_menu_handler>(
      state, 1, param_type, type_off, std::vector<std::vector<int>>({{ param_target, param_source }})); 
  };
  result.midi_active_selector_ = [cv, global, on_note_midi_start, sm = source_matrix.mappings](
    plugin_state const& state, int, jarray<int, 3>& active) { 
      select_midi_active(state, cv, global, on_note_midi_start, sm, active); 
  };
  if(cv)
  {
    result.gui.tabbed_name = "CV-CV";
    result.engine_factory = [global, sm = source_matrix.mappings, tm = target_matrix.mappings](
      auto const& topo, int, int) {
        return std::make_unique<cv_cv_matrix_engine>(global, topo, sm, tm);
    };
  }
  else
  {
    result.gui.tabbed_name = "CV-Audio";
    result.engine_factory = [global, sm = source_matrix.mappings, tm = target_matrix.mappings](
      auto const& topo, int, int) { 
        return std::make_unique<cv_audio_matrix_engine>(global, topo, sm, tm);
    };
  }

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 13, gui_dimension::auto_size, 4, 4, 4, 4 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Op", param_type, route_count),
    make_param_dsp_input(!global, param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 0 }, make_label_none())));
  type.gui.tabular = true;
  type.gui.menu_handler_factory = [route_count](plugin_state* state) { 
    return make_matrix_param_menu_handler(state, 1, 0, route_count, type_mul_abs); };
  type.info.description = std::string("Selects operation and stacking mode.<br/>") +
    "Add: add source signal to target parameter.<br/>" + 
    "Mul: multiply target parameter by source signal.<br/>" +
    "AB (Add-Bipolar): treat source signal as bipolar, then add to target parameter.<br/>" + 
    "Abs (Absolute): modulate as-if the target parameter has the full [0, 1] range available, may cause clipping.<br/>" + 
    "Rel (Relative): modulate taking into account only the target parameter value, may cause clipping.<br/>" +
    "Stk (Stacked): modulate taking into account all previous modulation sources affecting the same parameter.";

  auto& source = result.params.emplace_back(make_param(
    make_topo_info_basic("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_input(!global, param_automate::automate), make_domain_item(source_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.tabular = true;
  source.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  source.gui.submenu = source_matrix.submenu;
  source.gui.enable_dropdown_drop_target = true;
  source.gui.drop_route_enabled_param_value = type_mul_abs;
  source.gui.drop_route_enabled_param_id = result.params[param_type].info.tag.id;
  source.info.description = std::string("All global CV and MIDI sources, plus for per-voice CV all per-voice CV sources, ") + 
    "MIDI note and velocity, and On-Note all global CV sources.";
  if (cv)
    source.gui.item_enabled.bind_param({ this_module, 0, param_target, gui_item_binding::match_param_slot },
      [global, sm = source_matrix.mappings, tm = target_matrix.mappings](int other, int self) {
        return sm[self].module_index < tm[other].module_index || 
          (sm[self].module_index == tm[other].module_index && sm[self].module_slot < tm[other].module_slot);
      });  
  
  auto& target = result.params.emplace_back(make_param(
    make_topo_info_basic("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_input(!global, param_automate::automate), make_domain_item(target_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.tabular = true;
  target.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  target.gui.submenu = target_matrix.submenu;
  target.gui.item_enabled.auto_bind = true;
  target.gui.enable_dropdown_drop_target = true;
  target.gui.drop_route_enabled_param_value = type_mul_abs;
  target.gui.drop_route_enabled_param_id = result.params[param_type].info.tag.id;
  if(cv)
    target.info.description = "Any modulatable parameter of any LFO or the CV-to-audio matrix. You can only route 'upwards', so not LFO2->LFO1.";
  else
    target.info.description = "Any modulatable parameter of any audio module, audio-to-audio matrix or (in case of per-voice) voice-in parameter.";
  if (cv)
    target.gui.item_enabled.bind_param({ this_module, 0, param_source, gui_item_binding::match_param_slot },
      [global, sm = source_matrix.mappings, tm = target_matrix.mappings](int other, int self) {
        return sm[other].module_index < tm[self].module_index ||
          (sm[other].module_index == tm[self].module_index && sm[other].module_slot < tm[self].module_slot);
      });

  auto& offset = result.params.emplace_back(make_param(
    make_topo_info("{86ECE946-D554-4445-B8ED-2A7380C910E4}", true, "Offset", "Off", "Off", param_offset, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-1, 1, 0, 2, ""),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 },
      make_label(cv? gui_label_contents::no_drag: gui_label_contents::drag, gui_label_align::left, gui_label_justify::center))));
  offset.gui.tabular = true;
  offset.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  offset.info.description = std::string("Source signal offset. Used to transform source before modulation is applied. ") +
    "Useful to stretch things like midi note/velocity into the full [0, 1] range.";
  auto& scale = result.params.emplace_back(make_param(
    make_topo_info("{6564CE04-0AB8-4CDD-8F3D-E477DD1F4715}", true, "Scale", "Scl", "Scl", param_scale, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(1, 32, 1, 2, ""),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 },
      make_label(cv ? gui_label_contents::no_drag : gui_label_contents::drag, gui_label_align::left, gui_label_justify::center))));
  scale.gui.tabular = true;
  scale.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  scale.info.description = std::string("Source signal multiplier. Used to transform source before modulation is applied. ") +
    "Useful to stretch things like midi note/velocity into the full [0, 1] range.";
  auto& min = result.params.emplace_back(make_param(
    make_topo_info_basic("{71E6F836-1950-4C8D-B62B-FAAD20B1FDBD}", "Min", param_min, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 5 },
      make_label(cv ? gui_label_contents::no_drag : gui_label_contents::drag, gui_label_align::left, gui_label_justify::center))));
  min.gui.tabular = true;
  min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  min.info.description = "Defines the bounds of the modulation effect. When min > max, modulation will invert.";
  auto& max = result.params.emplace_back(make_param(
    make_topo_info_basic("{DB3A5D43-95CB-48DC-97FA-984F55B57F7B}", "Max", param_max, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 6 },
      make_label(cv ? gui_label_contents::no_drag : gui_label_contents::drag, gui_label_align::left, gui_label_justify::center))));
  max.gui.tabular = true;
  max.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  max.info.description = "Defines the bounds of the modulation effect. When min > max, modulation will invert.";

  return result;
}

cv_matrix_engine_base::
cv_matrix_engine_base(
  bool cv, bool global, plugin_topo const& topo,
  std::vector<module_output_mapping> const& sources,
  std::vector<param_topo_mapping> const& targets):
_cv(cv), _global(global), _sources(sources), _targets(targets)
{
  plugin_dims dims(topo, topo.audio_polyphony);
  _mixdown.resize(dims.module_slot_param_slot);
  _modulation_indices.resize(dims.module_slot_param_slot);
}

void 
cv_cv_matrix_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{ *block.state.own_context = &_mixer; }

cv_cv_matrix_mixdown const& 
cv_cv_matrix_mixer::mix(plugin_block& block, int module, int slot)
{ return _engine->mix(block, module, slot); }

cv_cv_matrix_mixdown const&
cv_cv_matrix_engine::mix(plugin_block& block, int module, int slot)
{
  perform_mixdown(block, module, slot);
  return _mixdown[module][slot];
}

void
cv_audio_matrix_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  perform_mixdown(block, -1, -1);
  *block.state.own_context = &_mixdown;
}

void 
cv_matrix_engine_base::reset_audio(
  plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  // need to capture stuff here because when we start 
  // mixing "own" does not refer to us but to the caller
  // (in case of cv->cv modulation, cv->audio is good)
  _own_cv = &block->state.own_cv;
  _own_scratch = &block->state.own_scratch;
  _own_block_automation = &block->state.own_block_automation;
  _own_accurate_automation = &block->state.own_accurate_automation;
}

void
cv_matrix_engine_base::perform_mixdown(plugin_block& block, int module, int slot)
{
  // cv->cv matrix can modulate the cv->audio matrix
  cv_cv_matrix_mixer* mixer = nullptr;
  cv_cv_matrix_mixdown const* modulation = nullptr;
  int this_module = _global ? module_gcv_cv_matrix : module_vcv_cv_matrix;
  if (!_cv)
  {
    this_module = _global ? module_gcv_audio_matrix : module_vcv_audio_matrix;
    mixer = &get_cv_cv_matrix_mixer(block, _global);
    modulation = &mixer->mix(block, this_module, 0);
  }

  // set every modulatable parameter to its corresponding automation curve
  assert((module == -1) == (slot == -1));
  for (int m = 0; m < _targets.size(); m++)
    if (module == -1 || (_targets[m].module_index == module && _targets[m].module_slot == slot))
    {
      int tp = _targets[m].param_index;
      int tpi = _targets[m].param_slot;
      int tm = _targets[m].module_index;
      int tmi = _targets[m].module_slot;
      _modulation_indices[tm][tmi][tp][tpi] = -1;
      auto const& curve = block.state.all_accurate_automation[tm][tmi][tp][tpi];
      _mixdown[tm][tmi][tp][tpi] = &curve;
    }

  // apply modulation routing
  int modulation_index = 0;
  int route_count = route_count_from_matrix_type(_global, _cv);
  jarray<float, 1>* modulated_curve_ptrs[max_any_route_count] = { nullptr };
  mod_output_usage mod_output_usages[max_any_route_count];
  std::memset(&mod_output_usages, 0, sizeof(mod_output_usages));

  for (int r = 0; r < route_count; r++)
  {
    jarray<float, 1>* modulated_curve_ptr = nullptr;
    int type = (*_own_block_automation)[param_type][r].step();
    if (type == type_off) continue;

    // found out indices of modulation target
    int selected_target = (*_own_block_automation)[param_target][r].step();
    int tp = _targets[selected_target].param_index;
    int tpi = _targets[selected_target].param_slot;
    int tm = _targets[selected_target].module_index;
    int tmi = _targets[selected_target].module_slot;

    if (module != -1 && (tm != module || tmi != slot)) continue;
    auto const& target_curve = block.state.all_accurate_automation[tm][tmi][tp][tpi];

    // if already modulated, set target curve to own buffer
    int existing_modulation_index = _modulation_indices[tm][tmi][tp][tpi];
    if (existing_modulation_index != -1)
      modulated_curve_ptr = &(*_own_cv)[0][existing_modulation_index];
    else
    {
      // else pick the next of our own cv outputs
      modulated_curve_ptr = &(*_own_cv)[0][modulation_index];
      auto const& target_automation = block.state.all_accurate_automation[tm][tmi][tp][tpi];
      target_automation.copy_to(block.start_frame, block.end_frame, *modulated_curve_ptr);
      modulated_curve_ptrs[r] = modulated_curve_ptr;
      _mixdown[tm][tmi][tp][tpi] = modulated_curve_ptr;
      _modulation_indices[tm][tmi][tp][tpi] = modulation_index;
      
      // set up for the visuals
      mod_output_usages[modulation_index].in_use = true;
      mod_output_usages[modulation_index].modulated_curve_ptr = modulated_curve_ptr;
      mod_output_usages[modulation_index].module_global = block.plugin_desc_.module_topo_to_index.at(tm) + tmi;
      mod_output_usages[modulation_index].param_global = block.plugin_desc_.param_mappings.topo_to_index[tm][tmi][tp][tpi];
      modulation_index++;
    }

    assert(modulated_curve_ptr != nullptr);
    jarray<float, 1>& modulated_curve = *modulated_curve_ptr;

    // find out indices of modulation source
    int selected_source = (*_own_block_automation)[param_source][r].step();
    int sm = _sources[selected_source].module_index;
    int smi = _sources[selected_source].module_slot;
    int so = _sources[selected_source].output_index;
    int soi = _sources[selected_source].output_slot;
    auto const& source_curve = block.module_cv(sm, smi)[so][soi];

    // source must be regular unipolar otherwise add/bi/mul breaks
    for (int f = block.start_frame; f < block.end_frame; f++)
      check_unipolar(source_curve[f]);

    // pre-transform source signal, 
    // cv->cv matrix can modulate transformation params (scale/offset) of the cv->audio matrix
    jarray<float, 1> const* scale_curve_norm = nullptr;
    jarray<float, 1> const* offset_curve_norm = nullptr;
    if (_cv)
    {
      scale_curve_norm = &(*_own_accurate_automation)[param_scale][r];
      offset_curve_norm = &(*_own_accurate_automation)[param_offset][r];
    }
    else
    {
      scale_curve_norm = (*modulation)[param_scale][r];
      offset_curve_norm = (*modulation)[param_offset][r];
    }

    auto& scale_curve = (*_own_scratch)[scratch_scale];
    auto& offset_curve = (*_own_scratch)[scratch_offset];
    auto& transformed_source = (*_own_scratch)[scratch_transform_source];
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_scale, *scale_curve_norm, scale_curve);
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_offset, *offset_curve_norm, offset_curve);
    for (int f = block.start_frame; f < block.end_frame; f++)
      transformed_source[f] = std::clamp((offset_curve[f] + source_curve[f]) * scale_curve[f], 0.0f, 1.0f);

    // apply modulation
    // cv->cv matrix can modulate modulation params (min/max) of the cv->audio matrix
    jarray<float, 1> const* min_curve = nullptr;
    jarray<float, 1> const* max_curve = nullptr;
    if (_cv)
    {
      min_curve = &(*_own_accurate_automation)[param_min][r];
      max_curve = &(*_own_accurate_automation)[param_max][r];
    }
    else
    {
      min_curve = (*modulation)[param_min][r];
      max_curve = (*modulation)[param_max][r];
    }

    switch (type)
    {
    case type_mul_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] *= (*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f];
      break;
    case type_mul_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] = target_curve[f] + ((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]) * (1 - target_curve[f]);
      break;
    case type_mul_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] = modulated_curve[f] + ((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]) * (1 - modulated_curve[f]);
      break;
    case type_add_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f];
      break;
    case type_add_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - target_curve[f]) * ((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]);
      break;
    case type_add_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - modulated_curve[f]) * ((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]);
      break;
    case type_ab_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += unipolar_to_bipolar((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]) * 0.5f;
      break;
    case type_ab_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - std::fabs(0.5f - target_curve[f]) * 2.0f) * unipolar_to_bipolar((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]) * 0.5f;
      break;
    case type_ab_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - std::fabs(0.5f - modulated_curve[f]) * 2.0f) * unipolar_to_bipolar((*min_curve)[f] + ((*max_curve)[f] - (*min_curve)[f]) * transformed_source[f]) * 0.5f;
      break;
    default:
      assert(false);
      break;
    }
  }

  // clamp, modulated_curve_ptrs[r] will point to the first 
  // occurence of modulation, but mod effects are accumulated there
  for (int r = 0; r < route_count; r++)
    if (modulated_curve_ptrs[r] != nullptr)
      modulated_curve_ptrs[r]->transform(block.start_frame, block.end_frame, [](float v) { return std::clamp(v, 0.0f, 1.0f); });

  // push param modulation outputs 
  if (block.graph) return;

  for (int r = 0; r < route_count; r++)
    if (mod_output_usages[r].in_use)
    {
      // debugging
      auto const& param_desc = block.plugin_desc_.params[mod_output_usages[r].param_global];
      (void)param_desc;

      block.push_modulation_output(modulation_output::make_mod_output_param_state(
        _global ? -1 : block.voice->state.slot,
        mod_output_usages[r].module_global,
        mod_output_usages[r].param_global,
        (*mod_output_usages[r].modulated_curve_ptr)[block.end_frame - 1]));
    }
}

}
