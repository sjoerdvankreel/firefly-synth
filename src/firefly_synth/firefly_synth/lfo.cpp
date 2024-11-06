#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/gui/mseg_editor.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/noise_generator.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {
             
static int const mseg_max_seg_count = 16;
static float const max_filter_time_ms = 500; 
static float const log_half = std::log(0.5f);

// need for realtime animation
enum { custom_tag_ref_phase, custom_tag_rand_seed, custom_tag_end_value };

enum class lfo_stage { cycle, filter, end };
enum { scratch_rate, scratch_count };
enum { type_off, type_repeat, type_one_shot, type_one_phase };
enum { section_left, section_shared, section_non_mseg, section_mseg };
enum {
  param_type, param_rate, param_tempo,  // main
  param_phase, param_steps, param_smooth, // shared
  param_sync, param_snap, param_mseg_on, // shared
  param_shape, param_seed, param_voice_rnd_source, // non mseg
  param_skew_x, param_skew_x_amt, param_skew_y, param_skew_y_amt, // non mseg
  param_mseg_start_y, param_mseg_count, param_mseg_w, param_mseg_y, param_mseg_slope, param_mseg_snap_x, param_mseg_snap_y // mseg
};

static bool
is_noise_voice_rand(int shape)
{
  return shape == wave_shape_type_smooth_2 || 
    shape == wave_shape_type_smooth_free_2 ||
    shape == wave_shape_type_static_2 || 
    shape == wave_shape_type_static_free_2;
}

static bool
is_noise_not_voice_rand(int shape)
{
  return shape == wave_shape_type_smooth_1 ||
    shape == wave_shape_type_smooth_free_1 ||
    shape == wave_shape_type_static_1 ||
    shape == wave_shape_type_static_free_1;
}

static bool
is_noise_free_running(int shape)
{
  return shape == wave_shape_type_static_free_1 ||
    shape == wave_shape_type_static_free_2 ||
    shape == wave_shape_type_smooth_free_1 ||
    shape == wave_shape_type_smooth_free_2;
}

static bool
is_noise_static(int shape)
{
  return shape == wave_shape_type_static_1 ||
    shape == wave_shape_type_static_2 ||
    shape == wave_shape_type_static_free_1 ||
    shape == wave_shape_type_static_free_2;
}

static bool
is_noise(int shape) 
{ 
  return is_noise_voice_rand(shape) || 
    is_noise_not_voice_rand(shape); 
}

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{E8D04800-17A9-42AB-9CAE-19322A400334}", "Off");
  result.emplace_back("{F2B8F550-76EA-41B9-97C3-13C1CC4414F1}", "Repeat");
  result.emplace_back("{BAE7EAC9-D7D2-4DD9-B550-C0ABC09F1C0D}", "One Shot");
  result.emplace_back("{377AF4E8-0FD8-4CC2-B9D1-4DB4F2FC114A}", "One Phase");
  return result;
}

class lfo_state_converter :
public state_converter
{
  bool const _global;
  plugin_desc const* const _desc;
public:
  lfo_state_converter(plugin_desc const* const desc, bool global) : _desc(desc), _global(global) {}
  void post_process_always(load_handler const& handler, plugin_state& new_state) override {}
  void post_process_existing(load_handler const& handler, plugin_state& new_state) override;

  bool handle_invalid_param_value(
    std::string const& new_module_id, int new_module_slot,
    std::string const& new_param_id, int new_param_slot,
    std::string const& old_value, load_handler const& handler,
    plain_value& new_value) override;
};

class lfo_engine :
public module_engine {

  float _phase;
  float _ref_phase;
  float _lfo_end_value;
  float _filter_end_value;
  
  bool const _global;
  lfo_stage _stage = {};
  cv_filter _filter = {};

  // noise
  noise_generator<true> _smooth_noise;
  noise_generator<false> _static_noise;

  // mseg - we deal with mseg by filling these out
  // at voice start (VLFO) or block start (GLFO)
  // then indexing the resulting segments by the phase
  int _mseg_seg_count = 0;
  float _mseg_start_y = 0;
  std::array<float, mseg_max_seg_count> _mseg_y = {};
  std::array<float, mseg_max_seg_count> _mseg_exp = {};
  std::array<float, mseg_max_seg_count> _mseg_time = {};

  int _end_filter_pos = 0;
  int _end_filter_stage_samples = 0;

  int _per_voice_seed = -1;
  int _prev_global_seed = -1;
  int _prev_global_type = -1;
  int _prev_global_steps = -1;
  int _prev_global_shape = -1;
  bool _prev_global_snap = false;
  bool _voice_was_initialized = false;

  // graphing
  bool _noise_graph_was_init = false;

  void update_block_params(plugin_block const* block);
  template <bool GlobalUnison>
  void process_uni(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type>
  void process_uni_type(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type, bool Sync>
  void process_uni_type_sync(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap>
  void process_uni_type_sync_snap(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape>
  void process_uni_type_sync_snap_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX>
  void process_uni_type_sync_snap_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX, class SkewY>
  void process_uni_type_sync_snap_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX, class SkewY, class Quantize>
  void process_uni_type_sync_snap_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize);
  template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Calc, class Quantize>
  void process_loop(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Calc calc, Quantize quantize);

  // without this it's annoying when switching between repeat/oneshot
  void reset_for_global(float new_phase, float phase_offset);

  // set the mseg curve
  void init_mseg(plugin_block& block, cv_cv_matrix_mixdown const* modulation);

  // process loop
  void process_internal(plugin_block& block, cv_cv_matrix_mixdown const* modulation);

public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  lfo_engine(bool global) : 
  _global(global), _smooth_noise(1, 1) {}

  void reset_audio(plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  void reset_graph(plugin_block const* block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes, 
    std::vector<mod_out_custom_state> const& custom_outputs, 
    void* context) override;

  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override { process_internal(block, nullptr); }
  void process_graph(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes, std::vector<mod_out_custom_state> const& custom_outputs, void* context) override
  { process_internal(block, static_cast<cv_cv_matrix_mixdown const*>(context)); }
};

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_glfo, 0, param_type, 0, "Repeat");
  state.set_text_at(module_glfo, 0, param_sync, 0, "On");
  state.set_text_at(module_glfo, 0, param_tempo, 0, "3/1");
  state.set_text_at(module_glfo, 1, param_type, 0, "Repeat");
  state.set_text_at(module_glfo, 1, param_sync, 0, "Off");
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = 200;
  return result;
}

static std::unique_ptr<graph_engine>
make_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

float
lfo_frequency_from_state(plugin_state const& state, int module_index, int module_slot, int bpm)
{
  bool sync = state.get_plain_at(module_index, module_slot, param_sync, 0).step() != 0;
  return sync_or_freq_from_state(state, 120, sync, module_index, module_slot, param_rate, param_tempo);
}

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param, 
  param_topo_mapping const& mapping, std::vector<mod_out_custom_state> const& custom_outputs)
{
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step();
  bool sync = state.get_plain_at(mapping.module_index, mapping.module_slot, param_sync, 0).step() != 0;
  if(type == type_off) return graph_data(graph_data_type::off, { state.desc().plugin->modules[mapping.module_index].info.tag.menu_display_name });
  
  int sample_rate = -1;
  std::string partition;
  auto const params = make_graph_engine_params();
  bool global = mapping.module_index == module_glfo;
  float freq = lfo_frequency_from_state(state, mapping.module_index, mapping.module_slot, 120);

  // make sure we exactly plot 1 cycle otherwise
  // we cannot know where to put the mod outputs
  sample_rate = params.max_frame_count * freq;
  if (!sync)
    partition = float_to_string(1 / freq, 2) + " Sec";
  else
  {
    // 1 bar, or the minimum that shows 1 cycle
    float one_bar_freq = timesig_to_freq(120, { 1, 1 });
    partition = float_to_string(one_bar_freq / freq, 2) + " Bar";
  }

  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  
  // we need this for the on-voice-random
  // although it's just mapped to fixed values
  // but nice to see the effect of source selection
  engine->process_default(module_voice_on_note, 0, custom_outputs, nullptr);

  auto const* block = engine->process(mapping.module_index, mapping.module_slot, custom_outputs, nullptr, [&](plugin_block& block) {
    lfo_engine engine(global);
    cv_cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block)[mapping.module_index][mapping.module_slot]);
    engine.reset_graph(&block, nullptr, nullptr, custom_outputs, &modulation);
    engine.process_graph(block, nullptr, nullptr, custom_outputs, &modulation);
  });
  engine->process_end();
  jarray<float, 1> series(block->state.own_cv[0][0]);
  return graph_data(series, false, 1.0f, false, { partition });
}

bool
lfo_state_converter::handle_invalid_param_value(
  std::string const& new_module_id, int new_module_slot,
  std::string const& new_param_id, int new_param_slot,
  std::string const& old_value, load_handler const& handler,
  plain_value& new_value)
{
  // note param ids are equal between vlfo/glfo, glfo just has more
  if (handler.old_version() < plugin_version{ 1, 2, 0 })
  {
    // smooth or fold -> sqr or fold
    if (new_param_id == _desc->plugin->modules[module_glfo].params[param_shape].info.tag.id)
    {
      // format is {guid}-{guid}-{guid}
      if (old_value.size() != 3 * 38 + 2) return false;
      auto shape_items = wave_shape_type_items(wave_target::lfo, _global);
      std::string old_shape_guid = old_value.substr(0, 38);
      
      // was smooth or fold
      if (old_shape_guid == "{E16E6DC4-ACB3-4313-A094-A6EA9F8ACA85}")
      {
        new_value = _desc->raw_to_plain_at(module_glfo, param_shape, wave_shape_type_smooth_1);
        return true;
      }

      // was sqr
      if (old_shape_guid == "{7176FE9E-D2A8-44FE-B312-93D712173D29}")
      {
        new_value = _desc->raw_to_plain_at(module_glfo, param_shape, wave_shape_type_sqr_or_fold);
        return true;
      }
    }

    // sync/rate + repeat/one-shot/one-phase got split out to separate controls
    if (new_param_id == _desc->plugin->modules[module_glfo].params[param_type].info.tag.id)
    {
      if (old_value == "{5F57863F-4157-4F53-BB02-C6693675B881}" || old_value == "{E2692483-F48B-4037-BF74-64BB62110538}")
      {
        // repeat
        new_value = _desc->raw_to_plain_at(module_glfo, param_type, type_repeat);
        return true;
      }
      if (old_value == "{0A5F479F-9180-4498-9464-DBEA0595C86B}" || old_value == "{85B1AC0B-FA06-4E23-A7EF-3EBF6F620948}")
      {
        // one shot
        new_value = _desc->raw_to_plain_at(module_glfo, param_type, type_one_shot);
        return true;
      }
      if (old_value == "{12E9AF37-1C1F-43AB-9405-86F103293C4C}" || old_value == "{9CFBC6ED-1024-4FDE-9291-9280FDA9BC1E}")
      {
        // one phase
        new_value = _desc->raw_to_plain_at(module_glfo, param_type, type_one_phase);
        return true;
      }
    }

    // Shape+SkewX/SkewY got split out to separate shape/skew x/skew y controls
    if (new_param_id == _desc->plugin->modules[module_glfo].params[param_shape].info.tag.id)
    {
      // format is {guid}-{guid}-{guid}
      if (old_value.size() != 3 * 38 + 2) return false;
      auto shape_items = wave_shape_type_items(wave_target::lfo, _global);
      std::string old_shape_guid = old_value.substr(0, 38);
      for (int i = 0; i < shape_items.size(); i++)
        if (old_shape_guid == shape_items[i].id)
        {
          new_value = _desc->raw_to_plain_at(module_glfo, param_shape, i);
          return true;
        }
    }
  }
  return false;
}

void
lfo_state_converter::post_process_existing(load_handler const& handler, plugin_state& new_state)
{
  std::string old_value;
  int this_module = _global ? module_glfo : module_vlfo;
  auto const& modules = new_state.desc().plugin->modules;
  std::string module_id = modules[this_module].info.tag.id;
  if (handler.old_version() < plugin_version{ 1, 2, 0 })
  {
    auto skew_items = wave_skew_type_items();
    for (int i = 0; i < modules[this_module].info.slot_count; i++)
    {
      // pick up sync type from old repeat+sync
      if (handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_type].info.tag.id, 0, old_value))
      {
        // Rate
        if (old_value == "{5F57863F-4157-4F53-BB02-C6693675B881}" || 
            old_value == "{0A5F479F-9180-4498-9464-DBEA0595C86B}" || 
            old_value == "{12E9AF37-1C1F-43AB-9405-86F103293C4C}")
          new_state.set_plain_at(this_module, i, param_sync, 0,
            _desc->raw_to_plain_at(this_module, param_sync, 0));
        // Tempo synced
        if (old_value == "{E2692483-F48B-4037-BF74-64BB62110538}" ||
          old_value == "{85B1AC0B-FA06-4E23-A7EF-3EBF6F620948}" ||
          old_value == "{9CFBC6ED-1024-4FDE-9291-9280FDA9BC1E}")
          new_state.set_plain_at(this_module, i, param_sync, 0,
            _desc->raw_to_plain_at(this_module, param_sync, 1));
      }

      // pick up skew x/skew y type from old combined shape + skew x/y
      if (handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_shape].info.tag.id, 0, old_value))
      {
        // format is {guid}-{guid}-{guid}
        if (old_value.size() == 3 * 38 + 2)
        {
          std::string old_skew_x_guid = old_value.substr(38 + 1, 38);
          std::string old_skew_y_guid = old_value.substr(2 * 38 + 2, 38);
          for (int j = 0; j < skew_items.size(); j++)
          {
            if (skew_items[j].id == old_skew_x_guid)
              new_state.set_plain_at(this_module, i, param_skew_x, 0,
                _desc->raw_to_plain_at(this_module, param_skew_x, j));
            if (skew_items[j].id == old_skew_y_guid)
              new_state.set_plain_at(this_module, i, param_skew_y, 0,
                _desc->raw_to_plain_at(this_module, param_skew_y, j));
          }
        }
      }
    }
  }
}

module_topo
lfo_topo(int section, gui_position const& pos, bool global, bool is_fx)
{
  auto const voice_info = make_topo_info("{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}", true, "Voice LFO", "Voice LFO", "VLFO", module_vlfo, 10);
  auto const global_info = make_topo_info("{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", true, "Global LFO", "Global LFO", "GLFO", module_glfo, 10);
  module_stage stage = global ? module_stage::input : module_stage::voice;
  auto info = topo_info(global ? global_info : voice_info);
  info.description = std::string("Optional tempo-synced LFO with repeating and one-shot types, various periodic waveforms, smooth noise, ") +  
    "static noise, smoothing control, phase andjustment, stair-stepping " +
    "and horizontal and vertical skewing controls with various types.";

  std::vector<int> column_sizes = { 32, 47, 63 };
  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 1, {
      make_module_dsp_output(true, -1, make_topo_info_basic("{197CB1D4-8A48-4093-A5E7-2781C731BBFC}", "Output", 0, 1)) }),
    make_module_gui(section, pos, { { 1 }, column_sizes })));
  result.gui.tabbed_name = "LFO";
  result.gui.is_drag_mod_source = true;
  
  result.graph_engine_factory = make_graph_engine;
  if(global && !is_fx) result.default_initializer = init_global_default;
  result.graph_renderer = render_graph;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [global](auto const&, int, int) { return std::make_unique<lfo_engine>(global); };
  result.state_converter_factory = [global](auto desc) { return std::make_unique<lfo_state_converter>(desc, global); };

  result.sections.emplace_back(make_param_section(section_left,
    make_topo_tag_basic("{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Left"),
    make_param_section_gui({ 0, 0, 1, 1 }, gui_dimension({ 1, 1 }, { { gui_dimension::auto_size_all, 1 } }), gui_label_edit_cell_split::horizontal)));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{252D76F2-8B36-4F15-94D0-2E974EC64522}", "Type", param_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(type_items(), ""),
    make_param_gui_single(section_left, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = std::string("Selects repeating or one-shot type. ") +
    "For regular one-shot type, the LFO stays at it's end value after exactly 1 cycle. " + 
    "For phase one-shot type, the end value takes the phase offset parameter into account.";
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info_basic("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(0.01, 20, 1, 1, 2, "Hz"),
    make_param_gui_single(section_left, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  rate.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] == 0; });
  rate.info.description = "LFO rate in Hz.";
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Tempo", param_tempo, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }),
    make_param_gui_single(section_left, gui_edit_type::list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  tempo.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] != 0; });
  tempo.info.description = "LFO rate in bars.";

  result.sections.emplace_back(make_param_section(section_shared,
    make_topo_tag_basic("{3184E473-F733-48B8-ACC7-A5989DDB6925}", "Shared"),
    make_param_section_gui({ 0, 1, 1, 1 }, gui_dimension({ 1, 1 }, 
      { gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1 }),
      gui_label_edit_cell_split::horizontal)));
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", true, "Phase Offset", "Phase", "Phase", param_phase, 1),
    make_param_dsp(param_direction::input, global ? param_rate::block : param_rate::voice, param_automate::automate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_shared, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  phase.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  phase.info.description = "In per-voice module, allows for phase adjustment of periodic generators.";
  auto& steps = result.params.emplace_back(make_param(
    make_topo_info_basic("{445CF696-0364-4638-9BD5-3E1C9A957B6A}", "Steps", param_steps, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 99, 1, 0),
    make_param_gui_single(section_shared, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  steps.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  steps.info.description = std::string("Step count for static and smooth noise generators, set to > 1. ") +
    "Stair-stepping for periodic generators. Set to 1 for continuous or > 1 for stair-stepping.";
  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", param_smooth, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_linear(0, max_filter_time_ms, 0, 0, "Ms"),
    make_param_gui_single(section_shared, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  smooth.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  smooth.info.description = "Applies a lowpass filter to smooth out rough edges.";
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{7F59C0F3-739E-4068-B1FD-B1520775FFBA}", true, "Tempo Sync", "Sync", "Sync", param_sync, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_toggle(false),
    make_param_gui_single(section_shared, gui_edit_type::toggle, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  sync.info.description = "Toggles time or tempo-synced type.";
  auto& snap = result.params.emplace_back(make_param(
    make_topo_info("{B97DF7D3-3259-4343-9577-858C6A5B786B}", true, "Snap To Project", "Snap", "Snap", param_snap, 1),
    make_param_dsp(param_direction::input, global? param_rate::block: param_rate::voice, param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_shared, gui_edit_type::toggle, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  snap.gui.bindings.enabled.bind_params({ param_type }, [global](auto const& vs) { return global && vs[0] != type_off; });
  snap.info.description = "In global module, snaps lfo phase to project/song time. Note this defeats rate modulation!";
  auto& mseg_on = result.params.emplace_back(make_param(
    make_topo_info("{B38CCF7E-16E6-4359-8817-205C8AC21A04}", true, "MSEG On", "MSEG", "MSEG On", param_mseg_on, 1),
    make_param_dsp(param_direction::input, global ? param_rate::block : param_rate::voice, param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_shared, gui_edit_type::toggle, { 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mseg_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  mseg_on.info.description = "Toggles MSEG mode on/off.";

  auto& non_mseg_section = result.sections.emplace_back(make_param_section(section_non_mseg,
    make_topo_tag_basic("{6DE1B08B-6C81-4146-B752-02F9559EA8CE}", "Non MSEG"),
    make_param_section_gui({ 0, 2, 1, 1 }, gui_dimension({ 1, 1 }, 
      { gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, gui_dimension::auto_size_all }), gui_label_edit_cell_split::horizontal)));
  non_mseg_section.gui.autofit_row = 0;
  non_mseg_section.gui.bindings.visible.bind_params({ param_mseg_on }, [](auto const& vs) { return vs[0] == 0; });
  auto& shape = result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", true, "Shape", "Shp", "Shape", param_shape, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_shape_type_items(wave_target::lfo, global), "Sin"),
    make_param_gui_single(section_non_mseg, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  shape.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  shape.info.description = std::string("Selects waveform: various periodic functions plus smooth and static noise.");
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info("{19ED9A71-F50A-47D6-BF97-70EA389A62EA}", true, "Seed", "Sed", "Seed", param_seed, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_non_mseg, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  seed.gui.bindings.visible.bind_params({ param_type, param_shape, param_mseg_on }, [](auto const& vs) { return !is_noise_voice_rand(vs[1]); });
  seed.gui.bindings.enabled.bind_params({ param_type, param_shape, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && is_noise_not_voice_rand(vs[1]) && vs[2] == 0; });
  seed.info.description = "Seed value for static and smooth noise generators.";
  auto& voice_rnd_source = result.params.emplace_back(make_param(
    make_topo_info("{81DAE640-815C-4D61-8DDE-D4CAD70309EF}", true, "Source", "Src", "Source", param_voice_rnd_source, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(0, on_voice_random_count - 1, 1, 1),
    make_param_gui_single(section_non_mseg, gui_edit_type::list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  voice_rnd_source.gui.bindings.visible.bind_params({ param_type, param_shape, param_mseg_on }, [global](auto const& vs) { return !global && is_noise_voice_rand(vs[1]); });
  voice_rnd_source.gui.bindings.enabled.bind_params({ param_type, param_shape, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && is_noise_voice_rand(vs[1]) && vs[2] == 0; });
  voice_rnd_source.info.description = "Per-voice random stream source for static and smooth noise generators.";
  auto& x_mode = result.params.emplace_back(make_param(
    make_topo_info("{A95BA410-6777-4386-8E86-38B5CBA3D9F1}", true, "Skew X Mode", "Skew X", "Skew X", param_skew_x, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_non_mseg, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  x_mode.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  x_mode.info.description = "Horizontal skew mode.";
  auto& x_amt = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", true, "Skew X Amt", "Amt", "Skew X Amt", param_skew_x_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_non_mseg, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  x_amt.gui.bindings.enabled.bind_params({ param_type, param_skew_x, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != wave_skew_type_off && vs[2] == 0; });
  x_amt.info.description = "Horizontal skew amount.";
  auto& y_mode = result.params.emplace_back(make_param(
    make_topo_info("{5D716AA7-CAE6-4965-8FC1-345DAA7141B6}", true, "Skew Y Mode", "Skew Y", "Skew Y", param_skew_y, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_non_mseg, gui_edit_type::autofit_list, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  y_mode.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  y_mode.info.description = "Vertical skew mode.";
  auto& y_amt = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", true, "Skew Y Amt", "Amt", "Skew Y Amt", param_skew_y_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_non_mseg, gui_edit_type::knob, { 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  y_amt.gui.bindings.enabled.bind_params({ param_type, param_skew_y, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != wave_skew_type_off && vs[2] == 0; });
  y_amt.info.description = "Vertical skew amount.";

  auto& mseg_section = result.sections.emplace_back(make_param_section(section_mseg,
    make_topo_tag_basic("{FF935A6F-7099-4EC2-B289-D04E8A96505A}", "MSEG"),
    make_param_section_gui({ 0, 2, 1, 1 }, { 1, 1 })));
  mseg_section.gui.bindings.visible.bind_params({ param_mseg_on }, [](auto const& vs) { return vs[0] != 0; });
  mseg_section.gui.custom_gui_factory = [global](plugin_gui* gui, lnf* lnf, int module_slot, component_store store) {
    return &store_component<mseg_editor>(
      store, gui, lnf, global? module_glfo: module_vlfo, module_slot, param_mseg_start_y, param_mseg_count, -1,
      param_mseg_w, param_mseg_y, param_mseg_slope, param_mseg_snap_x, param_mseg_snap_y, false); };
  auto& mseg_start_y = result.params.emplace_back(make_param(
    make_topo_info_basic("{25C93688-3623-48FC-BCBC-B4418227737D}", "MSEG Start Y", param_mseg_start_y, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.0, 2, ""),
    make_param_gui_none(section_mseg)));
  mseg_start_y.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_start_y.info.description = "MSEG generator start level.";
  auto& mseg_count = result.params.emplace_back(make_param(
    make_topo_info_basic("{88C48255-B01F-44AD-9DB2-E476A1D095A6}", "MSEG Count", param_mseg_count, 1),
    make_param_dsp(param_direction::input, global? param_rate::block: param_rate::voice, param_automate::none), make_domain_step(1, mseg_max_seg_count, 3, 0),
    make_param_gui_none(section_mseg)));
  mseg_count.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_count.info.description = "MSEG generator segment count.";
  auto& mseg_w = result.params.emplace_back(make_param(
    make_topo_info_basic("{4FE48630-82CF-4189-979C-E50C9C2E646B}", "MSEG Width", param_mseg_w, mseg_max_seg_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(1.0, 100.0, 10.0, 2, ""),
    make_param_gui_none(section_mseg)));
  mseg_w.gui.bindings.enabled.bind_params({ param_type, param_mseg_on, param_mseg_snap_x }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0 && vs[2] == 0; });
  mseg_w.info.description = "MSEG generator segment width.";
  auto& mseg_y = result.params.emplace_back(make_param(
    make_topo_info_basic("{DAE89769-1DD5-47FB-B872-A9BE8C0B3EA1}", "MSEG Y", param_mseg_y, mseg_max_seg_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.0, 2, ""),
    make_param_gui_none(section_mseg)));
  mseg_y.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_y.info.description = "MSEG generator segment level.";
  mseg_y.domain.default_selector_ = [](int, int s) {
    if (s == 0) return "100";
    if (s == 1) return "50";
    if (s == 2) return "0";
    return "0.0";
    };
  auto& mseg_slope = result.params.emplace_back(make_param(
    make_topo_info_basic("{BB75193D-CD39-4636-AA4C-CE7DB4E8AFDD}", "MSEG Slope", param_mseg_slope, mseg_max_seg_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 2, ""),
    make_param_gui_none(section_mseg)));
  mseg_slope.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_slope.info.description = "MSEG generator segment slope.";
  mseg_slope.domain.default_selector_ = [](int, int s) {
    // todo better defaults
    if (s == 0) return "25";
    if (s == 1) return "50";
    if (s == 2) return "75";
    return "0.0";
    };
  auto& mseg_snap_x = result.params.emplace_back(make_param(
    make_topo_info_basic("{AC412B7F-CA1D-4E75-B48A-8B150CD155D3}", "MSEG Snap X", param_mseg_snap_x, 1),
    make_param_dsp(param_direction::input, global ? param_rate::block : param_rate::voice, param_automate::none), make_domain_toggle(false),
    make_param_gui_none(section_mseg)));
  mseg_snap_x.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_snap_x.info.description = "MSEG generator horizontal snapping on/off.";
  auto& mseg_snap_y = result.params.emplace_back(make_param(
    make_topo_info_basic("{3DCA89D6-73EA-4FF0-87FA-5FB23D5D181F}", "MSEG Snap Y", param_mseg_snap_y, 1),
    make_param_dsp(param_direction::input, global ? param_rate::block : param_rate::voice, param_automate::none), make_domain_step(0, 16, 0, 0),
    make_param_gui_none(section_mseg)));
  mseg_snap_y.gui.bindings.enabled.bind_params({ param_type, param_mseg_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  mseg_snap_y.info.description = "MSEG generator vertical snapping grid size";

  return result;
}

static float 
lfo_quantize(float in, int steps)
{ 
  // input must be [0, 1)
  float out = (int)(std::clamp(in, 0.0f, 0.9999f) * steps) / (float)(steps - 1);
  return check_unipolar(out);
}

void 
lfo_engine::update_block_params(plugin_block const* block)
{
  auto const& block_auto = block->state.own_block_automation;
  float smooth = block_auto[param_smooth][0].real();
  _filter.init(block->sample_rate, smooth / 1000.0f);
}

void
lfo_engine::reset_for_global(float new_phase, float phase_offset)
{
  _phase = new_phase;
  _ref_phase = _phase;
  // need to derive the new actual phase
  _phase += phase_offset;
  _phase -= std::floor(_phase);
  _end_filter_pos = 0;
  _stage = lfo_stage::cycle;
}

void
lfo_engine::init_mseg(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool mseg_snap_x = block_auto[param_mseg_snap_x][0].step() != 0;
  _mseg_seg_count = block_auto[param_mseg_count][0].step();

  _mseg_start_y = (*(*modulation)[param_mseg_start_y][0])[block.start_frame];
  for (int i = 0; i < _mseg_seg_count; i++)
  {
    _mseg_y[i] = (*(*modulation)[param_mseg_y][i])[block.start_frame];
    _mseg_exp[i] = (float)mseg_exp((*(*modulation)[param_mseg_slope][i])[block.start_frame]);
    _mseg_time[i] = mseg_snap_x? 1.0 / _mseg_seg_count: (*(*modulation)[param_mseg_w][i])[block.start_frame];
  }

  // normalize so it sums up to 1 (eg 0.25, 0.25, 0.25, 0.25)
  if (!mseg_snap_x)
  {
    float mseg_total_size = 0.0f;
    for (int i = 0; i < _mseg_seg_count; i++)
      mseg_total_size += _mseg_time[i];
    for (int i = 0; i < _mseg_seg_count; i++)
      _mseg_time[i] /= mseg_total_size;
  }

  // now make it accumulated for easier lookup (eg 0.25 0.5 0.75 1.0)
  for (int i = 1; i < _mseg_seg_count; i++)
    _mseg_time[i] += _mseg_time[i - 1];
}

void 
lfo_engine::reset_graph(
  plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes,
  std::vector<mod_out_custom_state> const& custom_outputs, 
  void* context)
{
  reset_audio(block, nullptr, nullptr);

  int new_rand_seed = 0;
  float new_ref_phase = 0.0f;
  float new_end_value = 0.0f;
  bool is_render_for_cv_graph = false;

  bool seen_end_value = false;
  bool seen_ref_phase = false;
  bool seen_rand_seed = false;
  bool seen_render_for_cv_graph = false;

  // backwards loop, outputs are sorted, latest-in-time are at the end
  if (custom_outputs.size())
    for (int i = (int)custom_outputs.size() - 1; i >= 0; i--)
    {
      if (custom_outputs[i].module_global == block->module_desc_.info.global)
      {
        if (!seen_rand_seed && custom_outputs[i].tag_custom == custom_tag_rand_seed)
        {
          seen_rand_seed = true;
          new_rand_seed = custom_outputs[i].value_custom_int();
        }
        if (!seen_ref_phase && custom_outputs[i].tag_custom == custom_tag_ref_phase)
        {
          seen_ref_phase = true;
          new_ref_phase = custom_outputs[i].value_custom_float();
        }
        if (!seen_end_value && custom_outputs[i].tag_custom == custom_tag_end_value)
        {
          seen_end_value = true;
          new_end_value = custom_outputs[i].value_custom_float();
        }
      }
      if (!seen_render_for_cv_graph && custom_outputs[i].tag_custom == custom_out_shared_render_for_cv_graph)
      {
        is_render_for_cv_graph = true;
        seen_render_for_cv_graph = true;
      }
      if (seen_rand_seed && seen_ref_phase && seen_render_for_cv_graph && seen_end_value)
        break;
    }

  auto const& block_auto = block->state.own_block_automation;
  if (seen_ref_phase && is_render_for_cv_graph)
  {
    _ref_phase = new_ref_phase;
    _phase = _ref_phase;
    if (!_global || block_auto[param_snap][0].step() != 0)
    {
      // need to derive the new actual phase
      _phase += block_auto[param_phase][0].real();
      _phase -= std::floor(_phase);
    }
  }

  // should be always there for GLFO
  // for VLFO, when it's not running,
  // we fall back to static seed
  // this does the "right" thing for not-per-voice-seeded stuff
  // and has the side effect that the static seed is rendered
  // instead of the per-voice-seed when the vlfo is not running for 
  // but still better than plotting nothing
  if (!seen_rand_seed)
  {
    new_rand_seed = block_auto[param_seed][0].step();
    seen_rand_seed = true;
  }

  if (seen_rand_seed)
  {
    if(is_noise_static(block_auto[param_shape][0].step()))
      _static_noise.init(new_rand_seed, block_auto[param_steps][0].step());
    else
      _smooth_noise.init(new_rand_seed, block_auto[param_steps][0].step());
  }

  _noise_graph_was_init = true;
  if (seen_end_value && is_render_for_cv_graph)
  {
    _stage = lfo_stage::end;
    _filter_end_value = check_unipolar(new_end_value);
  }
}

void
lfo_engine::reset_audio(
  plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{ 
  _ref_phase = 0;
  _lfo_end_value = 0;
  _end_filter_pos = 0;
  _filter_end_value = 0;
  _stage = lfo_stage::cycle;
  _end_filter_stage_samples = 0;
  _per_voice_seed = -1;
  _noise_graph_was_init = false;
  _voice_was_initialized = false;
  _prev_global_seed = -1;
  _prev_global_type = -1;
  _prev_global_shape = -1;
  _prev_global_steps = -1;
  _prev_global_snap = false;

  update_block_params(block);
  auto const& block_auto = block->state.own_block_automation;
  _phase = (_global && block_auto[param_snap][0].step() == 0) ? 0 : block_auto[param_phase][0].real();

  // global unison
  if (!_global && block->voice->state.sub_voice_count > 1)
  {
    float glob_uni_phs_offset = block->state.all_block_automation[module_voice_in][0][voice_in_param_uni_lfo_phase][0].real();
    float voice_pos = (float)block->voice->state.sub_voice_index / (block->voice->state.sub_voice_count - 1.0f);
    _phase += voice_pos * glob_uni_phs_offset;
    _phase -= (int)_phase;
  }
}

void
lfo_engine::process_internal(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  // allow custom data for graphs
  if (modulation == nullptr)
  {
    int this_module = _global? module_glfo: module_vlfo;
    cv_cv_matrix_mixer& mixer = get_cv_cv_matrix_mixer(block, _global);
    modulation = &mixer.mix(block, this_module, block.module_slot);
  }
 
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  int shape = block_auto[param_shape][0].step();
  int seed = block_auto[param_seed][0].step();
  int steps = block_auto[param_steps][0].step();
  bool snap = block_auto[param_snap][0].step() != 0;
  bool is_mseg = block_auto[param_mseg_on][0].step() != 0;

  if (type == type_off)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  // only want the mod outputs for the actual audio engine
  if (!block.graph)
  {
    block.push_modulation_output(modulation_output::make_mod_output_cv_state(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      _ref_phase));
    block.push_modulation_output(modulation_output::make_mod_output_custom_state_float(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      custom_tag_ref_phase,
      _ref_phase));
    block.push_modulation_output(modulation_output::make_mod_output_custom_state_int(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      custom_tag_rand_seed,
      is_noise_static(shape) ? _static_noise.seed() : _smooth_noise.seed()));
  }

  // snap-to-project may cause the phase to reset for lfos that are already "ended"
  // as does changing of any of the block params
  if(_stage == lfo_stage::end && !_global)
  {
    // need to keep updating the ui in this case
    // as long as the phase is wrapping we'll push out
    // notifications but if we dont keep reporting the end
    // value then ui will reset (because it is not sticky)
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _filter_end_value);

    if (!block.graph)
      block.push_modulation_output(modulation_output::make_mod_output_custom_state_float(
        _global ? -1 : block.voice->state.slot,
        block.module_desc_.info.global,
        custom_tag_end_value,
        _filter_end_value));

    return; 
  }

  if(_global)
  {
    update_block_params(&block);
    init_mseg(block, modulation);
    if (type != _prev_global_type || seed != _prev_global_seed || steps != _prev_global_steps || shape != _prev_global_shape || snap != _prev_global_snap)
    {
      _prev_global_seed = seed;
      _prev_global_type = type;
      _prev_global_snap = snap;
      _prev_global_steps = steps;
      _prev_global_shape = shape;
      reset_for_global(0.0f, block_auto[param_phase][0].real());
      if (!_noise_graph_was_init)
      {
        _smooth_noise = noise_generator<true>(seed, steps);
        _static_noise = noise_generator<false>(seed, steps);
      }
    }
  }
  else
  {
    // cannot do this in reset() because we depend on output of the on-note module
    if (!_voice_was_initialized)
    {
      if (is_mseg)
      {
        init_mseg(block, modulation);
      } else if (is_noise(shape))
      {
        if (is_noise_not_voice_rand(shape))
          _per_voice_seed = block_auto[param_seed][0].step();
        else
        {
          int source_index = block_auto[param_voice_rnd_source][0].step();
          assert(0 <= source_index && source_index < on_voice_random_count);
          // noise generators hate zero seed
          float on_note_rnd_cv = block.module_cv(module_voice_on_note, 0)[on_voice_random_output_index][source_index][0];
          _per_voice_seed = (int)(1 + (on_note_rnd_cv * (RAND_MAX - 1)));
        }
        if (!_noise_graph_was_init)
        {
          _smooth_noise = noise_generator<true>(_per_voice_seed, steps);
          _static_noise = noise_generator<false>(_per_voice_seed, steps);
        }
      }
      _voice_was_initialized = true;
    }
  }

  if(!_global && block.voice->state.sub_voice_count > 1)
    process_uni<true>(block, modulation);
  else
    process_uni<false>(block, modulation);
}

template <bool GlobalUnison> void
lfo_engine::process_uni(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  switch (block.state.own_block_automation[param_type][0].step())
  {
  case type_repeat: process_uni_type<GlobalUnison, type_repeat>(block, modulation); break;
  case type_one_shot: process_uni_type<GlobalUnison, type_one_shot>(block, modulation); break;
  case type_one_phase: process_uni_type<GlobalUnison, type_one_phase>(block, modulation); break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, int Type> void
lfo_engine::process_uni_type(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  static_assert(Type != type_off);
  bool sync = block.state.own_block_automation[param_sync][0].step() != 0;
  if(sync) process_uni_type_sync<GlobalUnison, Type, true>(block, modulation);
  else process_uni_type_sync<GlobalUnison, Type, false>(block, modulation);
}

template <bool GlobalUnison, int Type, bool Sync> void
lfo_engine::process_uni_type_sync(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  bool snap = block.state.own_block_automation[param_snap][0].step() != 0;
  if (_global && snap) process_uni_type_sync_snap<GlobalUnison, Type, Sync, true>(block, modulation);
  else process_uni_type_sync_snap<GlobalUnison, Type, Sync, false>(block, modulation);
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap> void
lfo_engine::process_uni_type_sync_snap(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  // todo deal with the skew
  auto const& block_auto = block.state.own_block_automation;
  bool is_mseg = block_auto[param_mseg_on][0].step() != 0;
  if (is_mseg)
  {
    process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, true>(block, modulation, [this](float in) { 
      for (int i = 0; i < _mseg_seg_count; i++)
      {
        if (in <= _mseg_time[i] || i == _mseg_seg_count - 1)
        {
          float this_y = _mseg_y[i];
          float this_x = _mseg_time[i];
          float this_exp = _mseg_exp[i];
          float prev_x = i == 0 ? 0.0f : _mseg_time[i - 1];
          float prev_y = i == 0 ? _mseg_start_y : _mseg_y[i - 1];
          return prev_y + std::pow((in - prev_x) / (this_x - prev_x), this_exp) * (this_y - prev_y);
        }
      }
      assert(false);
      return 0.0f;
    });
    return;
  }

  int seed = _global ? _prev_global_seed : _per_voice_seed;
  switch (block_auto[param_shape][0].step())
  {
  case wave_shape_type_saw: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_saw); break;
  case wave_shape_type_tri: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_tri); break;
  case wave_shape_type_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin); break;
  case wave_shape_type_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos); break;
  case wave_shape_type_sqr_or_fold: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sqr); break;
  case wave_shape_type_sin_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_sin); break;
  case wave_shape_type_sin_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_cos); break;
  case wave_shape_type_cos_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_sin); break;
  case wave_shape_type_cos_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, wave_shape_uni_cos_cos_cos); break;
  case wave_shape_type_smooth_1:
  case wave_shape_type_smooth_2:
  case wave_shape_type_smooth_free_1:
  case wave_shape_type_smooth_free_2:
    process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, [this](float in) {
      return wave_shape_uni_custom(in, [this](float in) {
        return _smooth_noise.at(in); }); });
    break;
  case wave_shape_type_static_1:
  case wave_shape_type_static_2:
  case wave_shape_type_static_free_1:
  case wave_shape_type_static_free_2:
    process_uni_type_sync_snap_shape<GlobalUnison, Type, Sync, Snap, false>(block, modulation, [this, seed](float in) {
      return wave_shape_uni_custom(in, [this](float in) {
        return _static_noise.at(in); }); });
    break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape> void
lfo_engine::process_uni_type_sync_snap_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape)
{
  if constexpr (MSEG)
  {
    // no skewing support for mseg - could be done easily, but doesnt fit in the gui
    process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_off); 
    return;
  }
  else
  {
    switch (block.state.own_block_automation[param_skew_x][0].step())
    {
    case wave_skew_type_off: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_off); break;
    case wave_skew_type_lin: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_lin); break;
    case wave_skew_type_scu: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_scu); break;
    case wave_skew_type_scb: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_scb); break;
    case wave_skew_type_xpu: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_xpu); break;
    case wave_skew_type_xpb: process_uni_type_sync_snap_shape_x<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_xpb); break;
    default: assert(false); break;
    }
  }
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX> void
lfo_engine::process_uni_type_sync_snap_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x)
{
  if constexpr (MSEG)
  {
    // no skewing support for mseg - could be done easily, but doesnt fit in the gui
    process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, wave_skew_uni_off, wave_skew_uni_off);
    return;
  }
  else
  {
    switch (block.state.own_block_automation[param_skew_y][0].step())
    {
    case wave_skew_type_off: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_off); break;
    case wave_skew_type_lin: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_lin); break;
    case wave_skew_type_scu: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_scu); break;
    case wave_skew_type_scb: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_scb); break;
    case wave_skew_type_xpu: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_xpu); break;
    case wave_skew_type_xpb: process_uni_type_sync_snap_shape_xy<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, wave_skew_uni_xpb); break;
    default: assert(false); break;
    }
  }
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX, class SkewY> void
lfo_engine::process_uni_type_sync_snap_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y)
{
  auto const& block_auto = block.state.own_block_automation;
  int shaper = block_auto[param_shape][0].step();
  int step = block_auto[param_steps][0].step();
  bool quantize = !is_noise(shaper) && step != 1;
  if(quantize) process_uni_type_sync_snap_shape_xy_quantize<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, skew_y, lfo_quantize);
  else process_uni_type_sync_snap_shape_xy_quantize<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, shape, skew_x, skew_y, [](float in, int st) { return in; });
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Shape, class SkewX, class SkewY, class Quantize> void
lfo_engine::process_uni_type_sync_snap_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize)
{
  auto const& block_auto = block.state.own_block_automation;
  int sx = block_auto[param_skew_x][0].step();
  int sy = block_auto[param_skew_y][0].step();
  bool x_is_exp = wave_skew_is_exp(sx);
  bool y_is_exp = wave_skew_is_exp(sy);

  if (!x_is_exp && !y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) { 
      return wave_calc_uni(in, x, y, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, processor, quantize);
  }
  else if (!x_is_exp && y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) { 
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, x, py, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, processor, quantize);
  }
  else if (x_is_exp && !y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      return wave_calc_uni(in, px, y, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, processor, quantize);
  }
  else
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, px, py, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync, Snap, MSEG>(block, modulation, processor, quantize);
  }
}

template <bool GlobalUnison, int Type, bool Sync, bool Snap, bool MSEG, class Calc, class Quantize>
void lfo_engine::process_loop(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Calc calc, Quantize quantize)
{
  int this_module = _global ? module_glfo : module_vlfo;
  auto const& block_auto = block.state.own_block_automation;
  int steps = block_auto[param_steps][0].step();
  int shape = block_auto[param_shape][0].step();

  auto const& x_curve = *(*modulation)[param_skew_x_amt][0];
  auto const& y_curve = *(*modulation)[param_skew_y_amt][0];
  auto& rate_curve = block.state.own_scratch[scratch_rate];

  if constexpr (Sync)
  {
    timesig sig = get_timesig_param_value(block, this_module, param_tempo);
    rate_curve.fill(block.start_frame, block.end_frame, timesig_to_freq(block.host.bpm, sig));
  }
  else
  {
    auto const& rate_curve_plain = *(*modulation)[param_rate][0];
    block.normalized_to_raw_block<domain_type::log>(this_module, param_rate, rate_curve_plain, rate_curve);
  }

  if constexpr (GlobalUnison)
  {
    float voice_pos = unipolar_to_bipolar((float)block.voice->state.sub_voice_index / (block.voice->state.sub_voice_count - 1.0f));
    auto const& glob_uni_lfo_dtn_curve = block.state.all_accurate_automation[module_voice_in][0][voice_in_param_uni_lfo_dtn][0];
    for(int f = block.start_frame; f < block.end_frame; f++)
      rate_curve[f] *= 1 + (voice_pos * glob_uni_lfo_dtn_curve[f]);
  }

  // reset phase to project time for glfo
  if constexpr (Snap)
  {
    // graph block always starts at 0
    if (!block.graph)
    {
      // Rate can go zero by modulation.
      float rate0 = rate_curve[block.start_frame];
      if (rate0 > 0.0f)
      {
        float phase_offset = block_auto[param_phase][0].real();
        std::int64_t samples0 = (std::int64_t)(block.sample_rate / rate0);
        std::int64_t phase_frames = block.host.project_time % samples0;

        // if phase in samples lands after the one-shot time, dont reset!
        if constexpr (Type == type_repeat)
        {
          reset_for_global(phase_frames / (float)samples0, phase_offset);
        }
        else if constexpr (Type == type_one_shot)
        {
          if (block.host.project_time < samples0)
            reset_for_global(phase_frames / (float)samples0, phase_offset);
        }
        else if constexpr (Type == type_one_phase)
        {
          if(block.host.project_time < samples0 * phase_offset)
            reset_for_global(phase_frames / (float)samples0, phase_offset);
        }
      }
    }
  }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == lfo_stage::end)
    {
      block.state.own_cv[0][0][f] = _filter_end_value;
      continue;
    }

    if (_stage == lfo_stage::filter)
    {
      _filter_end_value = _filter.next(_lfo_end_value);
      block.state.own_cv[0][0][f] = _filter_end_value;
      if (_end_filter_pos++ >= _end_filter_stage_samples)
        _stage = lfo_stage::end;
      continue;
    }

    _lfo_end_value = quantize(calc(_phase, x_curve[f], y_curve[f]), steps);
    _filter_end_value = _filter.next(check_unipolar(_lfo_end_value));
    block.state.own_cv[0][0][f] = _filter_end_value;

    bool phase_wrapped = increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate);
    bool ref_wrapped = increment_and_wrap_phase(_ref_phase, rate_curve[f], block.sample_rate);
    bool ended = ref_wrapped && Type == type_one_shot || phase_wrapped && Type == type_one_phase;

    if (ref_wrapped && !block.graph)
      if (is_noise_free_running(shape))
        if (is_noise_static(shape))
          _static_noise.resample();
        else
          _smooth_noise.resample();

    if (ended)
    {
      _stage = lfo_stage::filter;
      float smooth_ms = block_auto[param_smooth][0].real();
      _end_filter_stage_samples = block.sample_rate * smooth_ms * 0.001;
    }
  }
}

}