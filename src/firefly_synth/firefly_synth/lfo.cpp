#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>

#include <firefly_synth/noise_smooth.hpp>
#include <firefly_synth/noise_static.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

static float const max_filter_time_ms = 500; 
static float const log_half = std::log(0.5f);

enum class lfo_stage { cycle, filter, end };
enum { scratch_rate, scratch_count };
enum { mode_off, mode_repeat, mode_one_shot, mode_one_phase }; // todo rename to type
enum { section_left_top, section_left_bottom, section_right_top, section_right_bottom };
enum { 
  param_mode, param_sync, param_rate, param_tempo, 
  param_skew_x, param_skew_x_amt, param_shape, param_skew_y, param_skew_y_amt,
  param_seed, param_steps, param_filter, param_phase };

static bool is_noise(int shape) {
  return shape == wave_shape_type_smooth || shape == wave_shape_type_static || shape == wave_shape_type_static_free; }

static std::vector<list_item>
mode_items()
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
  void post_process(load_handler const& handler, plugin_state& new_state) override;

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
  smooth_noise _smooth_noise;
  static_noise _static_noise;

  int _prev_seed = -1;
  int _end_filter_pos = 0;
  int _end_filter_stage_samples = 0;
  int _smooth_noise_total_pos = 0;
  int _smooth_noise_total_samples = 0;

  void reset_smooth_noise(int seed, int steps);
  void update_block_params(plugin_block const* block);
  float calc_smooth(float phase, int seed, int steps);

  template <int Mode>
  void process_mode(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <int Mode, bool Sync>
  void process_mode_sync(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape>
  void process_mode_sync_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape);
  template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX>
  void process_mode_sync_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x);
  template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX, class SkewY>
  void process_mode_sync_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y);
  template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX, class SkewY, class Quantize>
  void process_mode_sync_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize);
  template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Calc, class Quantize>
  void process_loop(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Calc calc, Quantize quantize);

public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  void reset(plugin_block const*) override;
  lfo_engine(bool global) : 
  _global(global), _smooth_noise(1, 1) {}

  void process(plugin_block& block) override { process(block, nullptr); }
  void process(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
};

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_glfo, 0, param_mode, 0, "Repeat");
  state.set_text_at(module_glfo, 0, param_sync, 0, "On");
  state.set_text_at(module_glfo, 0, param_tempo, 0, "3/1");
  state.set_text_at(module_glfo, 1, param_mode, 0, "Repeat");
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
  plugin_state const& state, graph_engine* engine, 
  int param, param_topo_mapping const& mapping)
{
  int mode = state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, mapping.param_slot).step();
  bool sync = state.get_plain_at(mapping.module_index, mapping.module_slot, param_sync, mapping.param_slot).step() != 0;
  if(mode == mode_off) return graph_data(graph_data_type::off, {});
  
  int sample_rate = -1;
  std::string partition;
  auto const params = make_graph_engine_params();
  bool global = mapping.module_index == module_glfo;
  float freq = lfo_frequency_from_state(state, mapping.module_index, mapping.module_slot, 120);

  if (!sync)
  {
    // 1 second, or the minimum that shows 1 cycle
    if (freq >= 1)
    {
      partition = "1 Sec";
      sample_rate = params.max_frame_count;
    }
    else
    {
      sample_rate = params.max_frame_count * freq;
      partition = float_to_string(1 / freq, 2) + " Sec";
    }
  }
  
  // draw synced 1/1 as full cycle
  if (sync)
  {
    // 1 bar, or the minimum that shows 1 cycle
    float one_bar_freq = timesig_to_freq(120, { 1, 1 });
    if(freq >= one_bar_freq)
    {
      partition = "1 Bar";
      sample_rate = one_bar_freq * params.max_frame_count;
    }
    else
    {
      sample_rate = params.max_frame_count * freq;
      partition = float_to_string(one_bar_freq / freq, 2) + " Bar";
    }
  }

  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  auto const* block = engine->process(mapping.module_index, mapping.module_slot, [global, mapping](plugin_block& block) {
    lfo_engine engine(global);
    engine.reset(&block);
    cv_cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block)[mapping.module_index][mapping.module_slot]);
    engine.process(block, &modulation);
  });
  engine->process_end();
  jarray<float, 1> series(block->state.own_cv[0][0]);
  return graph_data(series, false, 1.0f, { partition });
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
      auto shape_items = wave_shape_type_items(true);
      std::string old_shape_guid = old_value.substr(0, 38);
      
      // was smooth or fold
      if (old_shape_guid == "{E16E6DC4-ACB3-4313-A094-A6EA9F8ACA85}")
      {
        new_value = _desc->raw_to_plain_at(module_glfo, param_shape, wave_shape_type_smooth);
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
    if (new_param_id == _desc->plugin->modules[module_glfo].params[param_mode].info.tag.id)
    {
      if (old_value == "{5F57863F-4157-4F53-BB02-C6693675B881}" || old_value == "{E2692483-F48B-4037-BF74-64BB62110538}")
      {
        // repeat
        new_value = _desc->raw_to_plain_at(module_glfo, param_mode, mode_repeat);
        return true;
      }
      if (old_value == "{0A5F479F-9180-4498-9464-DBEA0595C86B}" || old_value == "{85B1AC0B-FA06-4E23-A7EF-3EBF6F620948}")
      {
        // one shot
        new_value = _desc->raw_to_plain_at(module_glfo, param_mode, mode_one_shot);
        return true;
      }
      if (old_value == "{12E9AF37-1C1F-43AB-9405-86F103293C4C}" || old_value == "{9CFBC6ED-1024-4FDE-9291-9280FDA9BC1E}")
      {
        // one phase
        new_value = _desc->raw_to_plain_at(module_glfo, param_mode, mode_one_phase);
        return true;
      }
    }

    // Shape+SkewX/SkewY got split out to separate shape/skew x/skew y controls
    if (new_param_id == _desc->plugin->modules[module_glfo].params[param_shape].info.tag.id)
    {
      // format is {guid}-{guid}-{guid}
      if (old_value.size() != 3 * 38 + 2) return false;
      auto shape_items = wave_shape_type_items(true);
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
lfo_state_converter::post_process(load_handler const& handler, plugin_state& new_state)
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
      // pick up sync mode from old repeat+sync
      if (handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_mode].info.tag.id, 0, old_value))
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

      // pick up skew x/skew y mode from old combined shape + skew x/y
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
lfo_topo(int section, gui_colors const& colors, gui_position const& pos, bool global, bool is_fx)
{
  auto const voice_info = make_topo_info("{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}", true, "Voice LFO", "Voice LFO", "VLFO", module_vlfo, 10);
  auto const global_info = make_topo_info("{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", true, "Global LFO", "Global LFO", "GLFO", module_glfo, 10);
  module_stage stage = global ? module_stage::input : module_stage::voice;
  auto info = topo_info(global ? global_info : voice_info);
  info.description = std::string("Optional tempo-synced LFO with repeating and one-shot modes, various periodic waveforms, smooth noise, ") +  
    "static noise and free-running static noise, smoothing control, phase andjustment, stair-stepping " +
    "and horizontal and vertical skewing controls with various modes.";

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 1, {
      make_module_dsp_output(true, make_topo_info_basic("{197CB1D4-8A48-4093-A5E7-2781C731BBFC}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1, 1 }, { 2, 7 } })));
  
  result.graph_engine_factory = make_graph_engine;
  if(global && !is_fx) result.default_initializer = init_global_default;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [global](auto const&, int, int) { return std::make_unique<lfo_engine>(global); };
  result.graph_renderer = [](auto const& state, auto* engine, int param, auto const& mapping) {
    return render_graph(state, engine, param, mapping); };
  result.state_converter_factory = [global](auto desc) { return std::make_unique<lfo_state_converter>(desc, global); };

  result.sections.emplace_back(make_param_section(section_left_top,
    make_topo_tag_basic("{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Left Top"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { { 1 } }))));
  auto& mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{252D76F2-8B36-4F15-94D0-2E974EC64522}", "Mode", param_mode, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_left_top, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  mode.info.description = std::string("Selects time or tempo-synced and repeating or one-shot mode. ") +
    "For regular one-shot mode, the LFO stays at it's end value after exactly 1 cycle. " + 
    "For phase one-shot mode, the end value takes the phase offset parameter into account.";

  result.sections.emplace_back(make_param_section(section_left_bottom,
    make_topo_tag_basic("{98869A27-5991-4BA3-9481-01636BDACDCB}", "Left Bottom"),
    make_param_section_gui({ 1, 0 }, gui_dimension({ 1 }, { { gui_dimension::auto_size, 1 } }))));
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{7F59C0F3-739E-4068-B1FD-B1520775FFBA}", true, "Tempo Sync", "Sync", "Sync", param_sync, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_toggle(false),
    make_param_gui_single(section_left_bottom, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sync.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  sync.info.description = "Toggles time or tempo-synced mode.";
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info_basic("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(0.01, 20, 1, 1, 2, "Hz"),
    make_param_gui_single(section_left_bottom, gui_edit_type::hslider, { 0, 1 }, make_label_none())));
  rate.gui.bindings.enabled.bind_params({ param_mode, param_sync }, [](auto const& vs) { return vs[0] != mode_off && vs[1] == 0; });
  rate.gui.bindings.visible.bind_params({ param_mode, param_sync }, [](auto const& vs) { return vs[1] == 0; });
  rate.info.description = "LFO rate in Hz.";
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Tempo", param_tempo, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }),
    make_param_gui_single(section_left_bottom, gui_edit_type::list, { 0, 1 }, make_label_none())));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_mode, param_sync }, [](auto const& vs) { return vs[0] != mode_off && vs[1] != 0; });
  tempo.gui.bindings.visible.bind_params({ param_mode, param_sync }, [](auto const& vs) { return vs[1] != 0; });
  tempo.info.description = "LFO rate in bars.";

  // Don't include the phase param for global lfo.
  result.sections.emplace_back(make_param_section(section_right_top,
    make_topo_tag_basic("{A5B5DC53-2E73-4C0B-9DD1-721A335EA076}", "Right Top"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1, gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));
  auto& x_mode = result.params.emplace_back(make_param(
    make_topo_info("{A95BA410-6777-4386-8E86-38B5CBA3D9F1}", true, "Skew X Mode", "Skew X", "Skew X", param_skew_x, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_right_top, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  x_mode.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  x_mode.info.description = "Horizontal skew mode.";
  auto& x_amt = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", true, "Skew X Amt", "Skew X", "Skew X", param_skew_x_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_right_top, gui_edit_type::hslider, { 0, 1 }, make_label_none())));
  x_amt.gui.bindings.enabled.bind_params({ param_mode, param_skew_x }, [](auto const& vs) { return vs[0] != mode_off && vs[1] != wave_skew_type_off; });
  x_amt.info.description = "Horizontal skew amount.";
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Shape", param_shape, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_shape_type_items(false), "Sin"),
    make_param_gui_single(section_right_top, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  type.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  type.info.description = std::string("Selects waveform plus horizontal and vertical skewing modes. ") +
    "Waveforms are various periodic functions plus smooth noise, static noise and free-running static noise. " +
    "Skewing modes are off (cpu efficient, so use it if you dont need the extra control), linear, scale unipolar/bipolar and exponential unipolar/bipolar.";
  auto& y_mode = result.params.emplace_back(make_param(
    make_topo_info("{5D716AA7-CAE6-4965-8FC1-345DAA7141B6}", true, "Skew Y Mode", "Skew Y", "Skew Y", param_skew_y, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_right_top, gui_edit_type::autofit_list, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  y_mode.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  y_mode.info.description = "Vertical skew mode.";
  auto& y_amt = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", true, "Skew Y Amt", "Skew Y", "Skew Y", param_skew_y_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_right_top, gui_edit_type::hslider, { 0, 4 }, make_label_none())));
  y_amt.gui.bindings.enabled.bind_params({ param_mode, param_skew_y }, [](auto const& vs) { return vs[0] != mode_off && vs[1] != wave_skew_type_off; });
  y_amt.info.description = "Vertical skew amount.";

  // Don't include the phase param for global lfo.
  std::vector<int> column_sizes(3, 1);
  if (!global) column_sizes.push_back(1);
  result.sections.emplace_back(make_param_section(section_right_bottom,
    make_topo_tag_basic("{898CC825-49AE-4A62-B7D8-76CE67D05F5C}", "Right Bottom"),
    make_param_section_gui({ 1, 1 }, gui_dimension({ 1 }, column_sizes))));
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info_basic("{19ED9A71-F50A-47D6-BF97-70EA389A62EA}", "Seed", param_seed, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_right_bottom, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  seed.gui.bindings.enabled.bind_params({ param_mode, param_shape }, [](auto const& vs) { return vs[0] != mode_off && is_noise(vs[1]); });
  seed.info.description = "Seed value for static and smooth noise generators.";
  auto& steps = result.params.emplace_back(make_param(
    make_topo_info_basic("{445CF696-0364-4638-9BD5-3E1C9A957B6A}", "Steps", param_steps, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 99, 1, 0),
    make_param_gui_single(section_right_bottom, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  steps.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  steps.info.description = std::string("Step count for static and smooth noise generators, set to > 1. ") + 
    "Stair-stepping for periodic generators. Set to 1 for continuous or > 1 for stair-stepping.";
  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", param_filter, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_linear(0, max_filter_time_ms, 0, 0, "Ms"),
    make_param_gui_single(section_right_bottom, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  smooth.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  smooth.info.description = "Applies a lowpass filter to smooth out rough edges.";

  if(global) return result;
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info_basic("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", "Phase", param_phase, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_right_bottom, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  phase.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  phase.info.description = "In per-voice module, allows for phase adjustment of periodic generators.";

  return result;
}

static float 
lfo_quantize(float in, int steps)
{ 
  // input must be [0, 1)
  float out = (int)(std::clamp(in, 0.0f, 0.9999f) * steps) / (float)(steps - 1);
  return check_unipolar(out);
}

float
lfo_engine::calc_smooth(float phase, int seed, int steps)
{
  float result = _smooth_noise.next(phase * steps);
  if(_smooth_noise_total_pos++ >= _smooth_noise_total_samples)
    reset_smooth_noise(seed, steps);
  return result;
}

void
lfo_engine::reset_smooth_noise(int seed, int steps)
{
  _smooth_noise_total_pos = 0;
  _smooth_noise = smooth_noise(seed, steps);
}

void 
lfo_engine::update_block_params(plugin_block const* block)
{
  auto const& block_auto = block->state.own_block_automation;
  float filter = block_auto[param_filter][0].real();
  _filter.set(block->sample_rate, filter / 1000.0f);
}

void
lfo_engine::reset(plugin_block const* block) 
{ 
  _ref_phase = 0;
  _lfo_end_value = 0;
  _end_filter_pos = 0;
  _filter_end_value = 0;
  _stage = lfo_stage::cycle;
  _end_filter_stage_samples = 0;
  
  update_block_params(block);
  auto const& block_auto = block->state.own_block_automation;
  _phase = _global? 0: block_auto[param_phase][0].real();
  _static_noise.reset(block_auto[param_seed][0].step());
  reset_smooth_noise(block_auto[param_seed][0].step(), block_auto[param_steps][0].step());
}

void
lfo_engine::process(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  // allow custom data for graphs
  if (modulation == nullptr)
  {
    int this_module = _global? module_glfo: module_vlfo;
    cv_cv_matrix_mixer& mixer = get_cv_cv_matrix_mixer(block, _global);
    modulation = &mixer.mix(block, this_module, block.module_slot);
  }
 
  auto const& block_auto = block.state.own_block_automation;
  int mode = block_auto[param_mode][0].step();
  if (mode == mode_off)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  if(_stage == lfo_stage::end)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _filter_end_value);
    return; 
  }

  int seed = block_auto[param_seed][0].step();
  int steps = block_auto[param_steps][0].step();
  if(_global)
  {
    update_block_params(&block);
    if (seed != _prev_seed)
    {
      _prev_seed = seed;
      _static_noise.reset(seed);
      reset_smooth_noise(seed, steps);
    }
  }

  switch (mode)
  {
  case mode_repeat: process_mode<mode_repeat>(block, modulation); break;
  case mode_one_shot: process_mode<mode_one_shot>(block, modulation); break;
  case mode_one_phase: process_mode<mode_one_phase>(block, modulation); break;
  default: assert(false); break;
  }
}

template <int Mode> void
lfo_engine::process_mode(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  static_assert(Mode != mode_off);
  bool sync = block.state.own_block_automation[param_sync][0].step() != 0;
  if(sync) process_mode_sync<Mode, true>(block, modulation);
  else process_mode_sync<Mode, false>(block, modulation);
}

template <int Mode, bool Sync> void
lfo_engine::process_mode_sync(plugin_block& block, cv_cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int seed = block_auto[param_seed][0].step();
  int steps = block_auto[param_steps][0].step();
  switch (block.state.own_block_automation[param_shape][0].step())
  {
  case wave_shape_type_saw: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_saw); break;
  case wave_shape_type_tri: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_tri); break;
  case wave_shape_type_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin); break;
  case wave_shape_type_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos); break;
  case wave_shape_type_sqr_or_fold: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sqr); break;
  case wave_shape_type_sin_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_sin); break;
  case wave_shape_type_sin_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_cos); break;
  case wave_shape_type_cos_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_sin); break;
  case wave_shape_type_cos_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_mode_sync_shape<Mode, Sync, false, false>(block, modulation, wave_shape_uni_cos_cos_cos); break;
  case wave_shape_type_smooth: process_mode_sync_shape<Mode, Sync, true, false>(block, modulation, [this, seed, steps](float in) {
    return wave_shape_uni_custom(in, [this, seed, steps](float in) {
      return calc_smooth(in, seed, steps); }); }); break;
  case wave_shape_type_static: process_mode_sync_shape<Mode, Sync, false, true>(block, modulation, [this, seed](float in) {
    return wave_shape_uni_custom(in, [this, seed](float in) {
      return _static_noise.next<false>(in, seed); }); }); break;
  case wave_shape_type_static_free: process_mode_sync_shape<Mode, Sync, false, true>(block, modulation, [this, seed](float in) {
    return wave_shape_uni_custom(in, [this, seed](float in) {
      return _static_noise.next<true>(in, seed); }); }); break;
  default: assert(false); break;
  }
}

template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape> void
lfo_engine::process_mode_sync_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape)
{
  switch (block.state.own_block_automation[param_skew_x][0].step())
  {
  case wave_skew_type_off: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_off); break;
  case wave_skew_type_lin: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_lin); break;
  case wave_skew_type_scu: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_scu); break;
  case wave_skew_type_scb: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_scb); break;
  case wave_skew_type_xpu: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_xpu); break;
  case wave_skew_type_xpb: process_mode_sync_shape_x<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, wave_skew_uni_xpb); break;
  default: assert(false); break;
  }
}

template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX> void
lfo_engine::process_mode_sync_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x)
{
  switch (block.state.own_block_automation[param_skew_y][0].step())
  {
  case wave_skew_type_off: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_off); break;
  case wave_skew_type_lin: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_lin); break;
  case wave_skew_type_scu: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_scu); break;
  case wave_skew_type_scb: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_scb); break;
  case wave_skew_type_xpu: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_xpu); break;
  case wave_skew_type_xpb: process_mode_sync_shape_xy<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, wave_skew_uni_xpb); break;
  default: assert(false); break;
  }
}

template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX, class SkewY> void
lfo_engine::process_mode_sync_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y)
{
  auto const& block_auto = block.state.own_block_automation;
  int shaper = block_auto[param_shape][0].step();
  int step = block_auto[param_steps][0].step();
  bool quantize = !is_noise(shaper) && step != 1;
  if(quantize) process_mode_sync_shape_xy_quantize<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, skew_y, lfo_quantize);
  else process_mode_sync_shape_xy_quantize<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, shape, skew_x, skew_y, [](float in, int st) { return in; });
}

template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Shape, class SkewX, class SkewY, class Quantize> void
lfo_engine::process_mode_sync_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize)
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
    process_loop<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, processor, quantize);
  }
  else if (!x_is_exp && y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) { 
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, x, py, shape, skew_x, skew_y); };
    process_loop<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, processor, quantize);
  }
  else if (x_is_exp && !y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      return wave_calc_uni(in, px, y, shape, skew_x, skew_y); };
    process_loop<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, processor, quantize);
  }
  else
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, px, py, shape, skew_x, skew_y); };
    process_loop<Mode, Sync, IsSmoothNoise, IsStaticNoise>(block, modulation, processor, quantize);
  }
}

template <int Mode, bool Sync, bool IsSmoothNoise, bool IsStaticNoise, class Calc, class Quantize>
void lfo_engine::process_loop(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Calc calc, Quantize quantize)
{
  int this_module = _global ? module_glfo : module_vlfo;
  auto const& block_auto = block.state.own_block_automation;
  int steps = block_auto[param_steps][0].step();

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
    normalized_to_raw_into_fast<domain_type::log>(block, this_module, param_rate, rate_curve_plain, rate_curve);
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

    static_assert(!IsStaticNoise || !IsSmoothNoise);
    if constexpr(IsStaticNoise)
      _static_noise.update(block.sample_rate, rate_curve[f], steps);
    if constexpr(IsSmoothNoise)
      _smooth_noise_total_samples = std::ceil(block.sample_rate / rate_curve[f]);

    _lfo_end_value = quantize(calc(_phase, x_curve[f], y_curve[f]), steps);
    _filter_end_value = _filter.next(check_unipolar(_lfo_end_value));
    block.state.own_cv[0][0][f] = _filter_end_value;

    bool phase_wrapped = increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate);
    bool ref_wrapped = increment_and_wrap_phase(_ref_phase, rate_curve[f], block.sample_rate);
    bool ended = ref_wrapped && Mode == mode_one_shot || phase_wrapped && Mode == mode_one_phase;
    if (ended)
    {
      _stage = lfo_stage::filter;
      float filter_ms = block_auto[param_filter][0].real();
      _end_filter_stage_samples = block.sample_rate * filter_ms * 0.001;
    }
  }
}

}
