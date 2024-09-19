#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/noise_generator.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {
                     
static float const max_filter_time_ms = 500; 
static float const log_half = std::log(0.5f);

// need for realtime animation
enum { custom_tag_ref_phase, custom_tag_rand_state, custom_tag_end_value };

enum class lfo_stage { cycle, filter, end };
enum { scratch_rate, scratch_count };
enum { type_off, type_repeat, type_one_shot, type_one_phase };
enum { section_left, section_sync, section_skew, section_shape, section_phase };
enum {
  param_type, param_rate, param_tempo, param_sync,
  param_skew_x, param_skew_x_amt, param_skew_y, param_skew_y_amt,
  param_shape, param_steps, param_seed, param_voice_rnd_source, param_filter, param_phase };

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

static bool
noise_needs_continuous_repaint(int shape)
{
  return is_noise_free_running(shape) ||
    is_noise_voice_rand(shape);
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
  float _graph_phase;
  float _lfo_end_value;
  float _filter_end_value;
  
  bool const _global;
  lfo_stage _stage = {};
  cv_filter _filter = {};
  noise_generator<true> _smooth_noise;
  noise_generator<false> _static_noise;

  int _end_filter_pos = 0;
  int _end_filter_stage_samples = 0;

  int _per_voice_seed = -1;
  int _prev_global_seed = -1;
  int _prev_global_steps = -1;
  int _prev_global_shape = -1;
  bool _per_voice_seed_was_initialized = false;

  // graphing
  float _new_ref_phase_for_graph = 0.0f;
  bool _is_render_for_cv_graph = false;
  bool _need_new_phase_for_graph = false;
  std::uint32_t _seed_resample_table_for_graph = 0;

  void update_block_params(plugin_block const* block);
  template <bool GlobalUnison>
  void process_uni(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type>
  void process_uni_type(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type, bool Sync>
  void process_uni_type_sync(plugin_block& block, cv_cv_matrix_mixdown const* modulation);
  template <bool GlobalUnison, int Type, bool Sync, class Shape>
  void process_uni_type_sync_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape);
  template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX>
  void process_uni_type_sync_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x);
  template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX, class SkewY>
  void process_uni_type_sync_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y);
  template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX, class SkewY, class Quantize>
  void process_uni_type_sync_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize);
  template <bool GlobalUnison, int Type, bool Sync, class Calc, class Quantize>
  void process_loop(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Calc calc, Quantize quantize);

  void process_internal(plugin_block& block, cv_cv_matrix_mixdown const* modulation);

public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  lfo_engine(bool global) : 
  _global(global), _smooth_noise(1, 1) {}

  void reset_audio(plugin_block const*) override;
  void reset_graph(plugin_block const* block, std::vector<mod_out_custom_state> const& custom_outputs, void* context) override;

  void process_audio(plugin_block& block) override { process_internal(block, nullptr); }
  void process_graph(plugin_block& block, std::vector<mod_out_custom_state> const& custom_outputs, void* context) override 
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
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, mapping.param_slot).step();
  bool sync = state.get_plain_at(mapping.module_index, mapping.module_slot, param_sync, mapping.param_slot).step() != 0;
  if(type == type_off) return graph_data(graph_data_type::off, {});
  
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
    engine.reset_graph(&block, custom_outputs, &modulation);
    engine.process_graph(block, custom_outputs, &modulation);
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
      auto shape_items = wave_shape_type_items(false, _global);
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
      auto shape_items = wave_shape_type_items(false, _global);
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
    "static noise and free-running static noise, smoothing control, phase andjustment, stair-stepping " +
    "and horizontal and vertical skewing controls with various types.";

  std::vector<int> column_sizes = { 32, 13, 34, 63 };
  if(!global) column_sizes = { 32, 13, 34, 55, 8 };
  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 1, {
      make_module_dsp_output(true, make_topo_info_basic("{197CB1D4-8A48-4093-A5E7-2781C731BBFC}", "Output", 0, 1)) }),
    make_module_gui(section, pos, { { 1, 1 }, column_sizes })));
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
    make_param_section_gui({ 0, 0, 2, 1 }, gui_dimension({ 1, 1 }, { { gui_dimension::auto_size_all, 1 } }), gui_label_edit_cell_split::horizontal)));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{252D76F2-8B36-4F15-94D0-2E974EC64522}", "Type", param_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(type_items(), ""),
    make_param_gui_single(section_left, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = std::string("Selects time or tempo-synced and repeating or one-shot type. ") +
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

  auto& sync_section = result.sections.emplace_back(make_param_section(section_sync,
    make_topo_tag_basic("{404ADE73-7C3D-4BEF-8B5A-2E6DE467AB37}", "Sync"),
    make_param_section_gui({ 0, 1, 2, 1 }, gui_dimension({ 1, 1 }, { { 1 } }), gui_label_edit_cell_split::vertical)));
  sync_section.gui.merge_with_section = section_skew;
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{7F59C0F3-739E-4068-B1FD-B1520775FFBA}", true, "Tempo Sync", "Sync", "Sync", param_sync, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_toggle(false),
    make_param_gui_single(section_sync, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::top, gui_label_justify::center))));
  sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  sync.info.description = "Toggles time or tempo-synced type.";

  auto& skew_section = result.sections.emplace_back(make_param_section(section_skew,
    make_topo_tag_basic("{6DE1B08B-6C81-4146-B752-02F9559EA8CE}", "Skew"),
    make_param_section_gui({ 0, 2, 2, 1 }, gui_dimension({1, 1}, {
    gui_dimension::auto_size_all, gui_dimension::auto_size_all, gui_dimension::auto_size_all }), gui_label_edit_cell_split::horizontal)));
  skew_section.gui.merge_with_section = section_sync;
  auto& x_mode = result.params.emplace_back(make_param(
    make_topo_info("{A95BA410-6777-4386-8E86-38B5CBA3D9F1}", true, "Skew X Mode", "Skew X", "Skew X", param_skew_x, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_skew, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  x_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  x_mode.info.description = "Horizontal skew mode.";
  auto& x_amt = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", true, "Skew X Amt", "Skew X", "Skew X", param_skew_x_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_skew, gui_edit_type::knob, { 0, 2 }, make_label_none())));
  x_amt.gui.bindings.enabled.bind_params({ param_type, param_skew_x }, [](auto const& vs) { return vs[0] != type_off && vs[1] != wave_skew_type_off; });
  x_amt.info.description = "Horizontal skew amount.";
  x_mode.gui.alternate_drag_param_id = x_amt.info.tag.id;
  auto& y_mode = result.params.emplace_back(make_param(
    make_topo_info("{5D716AA7-CAE6-4965-8FC1-345DAA7141B6}", true, "Skew Y Mode", "Skew Y", "Skew Y", param_skew_y, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_skew, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  y_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  y_mode.info.description = "Vertical skew mode.";
  auto& y_amt = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", true, "Skew Y Amt", "Skew Y", "Skew Y", param_skew_y_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_skew, gui_edit_type::knob, { 1, 2 }, make_label_none())));
  y_amt.gui.bindings.enabled.bind_params({ param_type, param_skew_y }, [](auto const& vs) { return vs[0] != type_off && vs[1] != wave_skew_type_off; });
  y_amt.info.description = "Vertical skew amount.";
  y_mode.gui.alternate_drag_param_id = y_amt.info.tag.id;

  auto& shape_section = result.sections.emplace_back(make_param_section(section_shape,
    make_topo_tag_basic("{A5B5DC53-2E73-4C0B-9DD1-721A335EA076}", "Right"),
    make_param_section_gui({ 0, 3, 2, 1 }, gui_dimension({ 1, 1 }, {
    gui_dimension::auto_size_all, gui_dimension::auto_size, gui_dimension::auto_size_all, 1 }), gui_label_edit_cell_split::horizontal)));
  if(!global) shape_section.gui.merge_with_section = section_phase;
  auto& shape = result.params.emplace_back(make_param(
    make_topo_info_basic("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Shape", param_shape, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_shape_type_items(false, global), "Sin"),
    make_param_gui_single(section_shape, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  shape.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  shape.info.description = std::string("Selects waveform plus horizontal and vertical skewing modes. ") +
    "Waveforms are various periodic functions plus smooth noise, static noise and free-running static noise. " +
    "Skewing modes are off (cpu efficient, so use it if you dont need the extra control), linear, scale unipolar/bipolar and exponential unipolar/bipolar.";
  auto& steps = result.params.emplace_back(make_param(
    make_topo_info_basic("{445CF696-0364-4638-9BD5-3E1C9A957B6A}", "Steps", param_steps, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 99, 1, 0),
    make_param_gui_single(section_shape, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  steps.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  steps.info.description = std::string("Step count for static and smooth noise generators, set to > 1. ") +
    "Stair-stepping for periodic generators. Set to 1 for continuous or > 1 for stair-stepping.";
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info_basic("{19ED9A71-F50A-47D6-BF97-70EA389A62EA}", "Seed", param_seed, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_shape, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  seed.gui.bindings.visible.bind_params({ param_type, param_shape }, [](auto const& vs) { return !is_noise_voice_rand(vs[1]); });
  seed.gui.bindings.enabled.bind_params({ param_type, param_shape }, [](auto const& vs) { return vs[0] != type_off && is_noise_not_voice_rand(vs[1]); });
  seed.info.description = "Seed value for static and smooth noise generators.";
  auto& voice_rnd_source = result.params.emplace_back(make_param(
    make_topo_info_basic("{81DAE640-815C-4D61-8DDE-D4CAD70309EF}", "Source", param_voice_rnd_source, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_step(0, on_voice_random_count - 1, 1, 1),
    make_param_gui_single(section_shape, gui_edit_type::list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  voice_rnd_source.gui.bindings.visible.bind_params({ param_type, param_shape }, [](auto const& vs) { return is_noise_voice_rand(vs[1]); });
  voice_rnd_source.gui.bindings.enabled.bind_params({ param_type, param_shape }, [](auto const& vs) { return vs[0] != type_off && is_noise_voice_rand(vs[1]); });
  voice_rnd_source.info.description = "Per-voice random stream source for static and smooth noise generators.";
  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", param_filter, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_linear(0, max_filter_time_ms, 0, 0, "Ms"),
    make_param_gui_single(section_shape, gui_edit_type::hslider, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  smooth.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  smooth.info.description = "Applies a lowpass filter to smooth out rough edges.";

  if(global) return result;

  auto& phase_section = result.sections.emplace_back(make_param_section(section_phase,
    make_topo_tag_basic("{8EB0A04C-5D69-4B0E-89BD-884BC2EFDFBE}", "Phase"),
    make_param_section_gui({ 0, 4, 2, 1 }, gui_dimension({ 1, 1 }, { 1 }), gui_label_edit_cell_split::vertical)));
  if (!global) phase_section.gui.merge_with_section = section_shape;
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info_basic("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", "Phs", param_phase, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_phase, gui_edit_type::knob, { 0, 0 },
    make_label(gui_label_contents::name, gui_label_align::top, gui_label_justify::center))));
  phase.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
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

void 
lfo_engine::update_block_params(plugin_block const* block)
{
  auto const& block_auto = block->state.own_block_automation;
  float filter = block_auto[param_filter][0].real();
  _filter.init(block->sample_rate, filter / 1000.0f);
}

void 
lfo_engine::reset_graph(
  plugin_block const* block, 
  std::vector<mod_out_custom_state> const& custom_outputs, 
  void* context)
{
  reset_audio(block);
  int new_rand_state = 0;
  float new_ref_phase = 0.0f;
  float new_end_value = 0.0f;
  bool is_render_for_cv_graph = false;

  bool seen_end_value = false;
  bool seen_ref_phase = false;
  bool seen_rand_state = false;
  bool seen_render_for_cv_graph = false;

  // backwards loop, outputs are sorted, latest-in-time are at the end
  if (custom_outputs.size())
    for (int i = (int)custom_outputs.size() - 1; i >= 0; i--)
    {
      if (custom_outputs[i].module_global == block->module_desc_.info.global)
      {
        if (!seen_rand_state && custom_outputs[i].tag_custom == custom_tag_rand_state)
        {
          seen_rand_state = true;
          new_rand_state = custom_outputs[i].value_custom;
        }
        if (!seen_ref_phase && custom_outputs[i].tag_custom == custom_tag_ref_phase)
        {
          seen_ref_phase = true;
          new_ref_phase = custom_outputs[i].value_custom / (float)std::numeric_limits<int>::max();
        }
        if (!seen_end_value && custom_outputs[i].tag_custom == custom_tag_end_value)
        {
          seen_end_value = true;
          new_end_value = custom_outputs[i].value_custom / (float)std::numeric_limits<int>::max();
        }
      }
      if (!seen_render_for_cv_graph && custom_outputs[i].tag_custom == custom_out_shared_render_for_cv_graph)
      {
        is_render_for_cv_graph = true;
        seen_render_for_cv_graph = true;
      }
      if (seen_rand_state && seen_ref_phase && seen_render_for_cv_graph && seen_end_value)
        break;
    }

  if (seen_ref_phase)
  {
    _need_new_phase_for_graph = true;
    _new_ref_phase_for_graph = new_ref_phase;
    _seed_resample_table_for_graph = new_rand_state;
    _is_render_for_cv_graph = is_render_for_cv_graph;
  }
  if (seen_end_value && _is_render_for_cv_graph)
  {
    _stage = lfo_stage::end;
    _filter_end_value = check_unipolar(new_end_value);
  }
}

void
lfo_engine::reset_audio(plugin_block const* block) 
{ 
  _ref_phase = 0;
  _graph_phase = 0;
  _lfo_end_value = 0;
  _end_filter_pos = 0;
  _filter_end_value = 0;
  _stage = lfo_stage::cycle;
  _end_filter_stage_samples = 0;
  _per_voice_seed = -1;
  _per_voice_seed_was_initialized = false;
  _prev_global_seed = -1;
  _prev_global_shape = -1;
  _prev_global_steps = -1;

  update_block_params(block);
  auto const& block_auto = block->state.own_block_automation;
  _phase = _global? 0: block_auto[param_phase][0].real();

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
  if (type == type_off)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  // only want the mod outputs for the actual audio engine
  if (!block.graph)
    block.push_modulation_output(modulation_output::make_mod_output_cv_state(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      type == type_repeat ? _ref_phase : _graph_phase));

  if(_stage == lfo_stage::end)
  {
    // need to keep updating the ui in this case
    // as long as the phase is wrapping we'll push out
    // notifications but if we dont keep reporting the end
    // value then ui will reset (because it is not sticky)
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _filter_end_value);
    if (block.graph) return;

    block.push_modulation_output(modulation_output::make_mod_output_custom_state(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      custom_tag_ref_phase,
      (int)(_ref_phase * std::numeric_limits<int>::max())));
    block.push_modulation_output(modulation_output::make_mod_output_custom_state(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      custom_tag_end_value,
      (int)(_filter_end_value * std::numeric_limits<int>::max())));
    return; 
  }

  int steps = block_auto[param_steps][0].step();
  int seed = block_auto[param_seed][0].step();
  int shape = block_auto[param_shape][0].step();

  if(_global)
  {
    update_block_params(&block);
    if (seed != _prev_global_seed || steps != _prev_global_steps || shape != _prev_global_shape)
    {
      _prev_global_seed = seed;
      _prev_global_steps = steps;
      _prev_global_shape = shape;
      _static_noise = noise_generator<false>(seed, steps);
      _smooth_noise = noise_generator<true>(seed, steps);

      if(!block.graph)
      {
        _need_new_phase_for_graph = true;
        _new_ref_phase_for_graph = _ref_phase;
        if(is_noise_static(shape))
          _seed_resample_table_for_graph = _static_noise.state();
        else
          _seed_resample_table_for_graph = _smooth_noise.state();
      }
    }
  }
  else
  {
    // cannot do this in reset() because we depend on output of the on-note module
    if (!_per_voice_seed_was_initialized)
    {
      if (is_noise(shape))
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
        _static_noise = noise_generator<false>(_per_voice_seed, steps);
        _smooth_noise = noise_generator<true>(_per_voice_seed, steps);
      }
      
      _per_voice_seed_was_initialized = true;
      if (!block.graph)
      {
        _need_new_phase_for_graph = true;
        _new_ref_phase_for_graph = _ref_phase;
        _seed_resample_table_for_graph = _per_voice_seed;
      }
    }
  }

  if (block.graph && _need_new_phase_for_graph)
  {
    if (_is_render_for_cv_graph)
    {
      _ref_phase = _new_ref_phase_for_graph;
      _phase = _ref_phase;

      if (!_global)
      {
        // need to derive the new actual phase
        _phase += block_auto[param_phase][0].real();
        _phase -= std::floor(_phase);
      }
    }
    if (noise_needs_continuous_repaint(shape))
    {
      if(is_noise_static(shape))
        _static_noise.sample_table(_seed_resample_table_for_graph);
      else
        _smooth_noise.sample_table(_seed_resample_table_for_graph);
    }
    _need_new_phase_for_graph = false;
  }

  if (!block.graph && _need_new_phase_for_graph)
  {
    block.push_modulation_output(modulation_output::make_mod_output_custom_state(
      _global ? -1 : block.voice->state.slot,
      block.module_desc_.info.global,
      custom_tag_ref_phase,
      (int)(_ref_phase * std::numeric_limits<int>::max())));
    if (noise_needs_continuous_repaint(shape))
      block.push_modulation_output(modulation_output::make_mod_output_custom_state(
        _global ? -1 : block.voice->state.slot,
        block.module_desc_.info.global,
        custom_tag_rand_state,
        _seed_resample_table_for_graph));
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
  int seed = _global ? _prev_global_seed : _per_voice_seed;
  switch (block.state.own_block_automation[param_shape][0].step())
  {
  case wave_shape_type_saw: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_saw); break;
  case wave_shape_type_tri: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_tri); break;
  case wave_shape_type_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin); break;
  case wave_shape_type_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos); break;
  case wave_shape_type_sqr_or_fold: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sqr); break;
  case wave_shape_type_sin_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_sin); break;
  case wave_shape_type_sin_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_cos); break;
  case wave_shape_type_cos_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_sin); break;
  case wave_shape_type_cos_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, wave_shape_uni_cos_cos_cos); break;
  case wave_shape_type_smooth_1: 
  case wave_shape_type_smooth_2:
  case wave_shape_type_smooth_free_1:
  case wave_shape_type_smooth_free_2:
    process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, [this](float in) {
      return wave_shape_uni_custom(in, [this](float in) {
        return _smooth_noise.at(in); }); }); 
    break;
  case wave_shape_type_static_1:
  case wave_shape_type_static_2:
  case wave_shape_type_static_free_1:
  case wave_shape_type_static_free_2:
    process_uni_type_sync_shape<GlobalUnison, Type, Sync>(block, modulation, [this, seed](float in) {
      return wave_shape_uni_custom(in, [this](float in) {
        return _static_noise.at(in); }); }); 
    break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, int Type, bool Sync, class Shape> void
lfo_engine::process_uni_type_sync_shape(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape)
{
  switch (block.state.own_block_automation[param_skew_x][0].step())
  {
  case wave_skew_type_off: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_off); break;
  case wave_skew_type_lin: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_lin); break;
  case wave_skew_type_scu: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_scu); break;
  case wave_skew_type_scb: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_scb); break;
  case wave_skew_type_xpu: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_xpu); break;
  case wave_skew_type_xpb: process_uni_type_sync_shape_x<GlobalUnison, Type, Sync>(block, modulation, shape, wave_skew_uni_xpb); break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX> void
lfo_engine::process_uni_type_sync_shape_x(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x)
{
  switch (block.state.own_block_automation[param_skew_y][0].step())
  {
  case wave_skew_type_off: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_off); break;
  case wave_skew_type_lin: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_lin); break;
  case wave_skew_type_scu: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_scu); break;
  case wave_skew_type_scb: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_scb); break;
  case wave_skew_type_xpu: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_xpu); break;
  case wave_skew_type_xpb: process_uni_type_sync_shape_xy<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, wave_skew_uni_xpb); break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX, class SkewY> void
lfo_engine::process_uni_type_sync_shape_xy(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y)
{
  auto const& block_auto = block.state.own_block_automation;
  int shaper = block_auto[param_shape][0].step();
  int step = block_auto[param_steps][0].step();
  bool quantize = !is_noise(shaper) && step != 1;
  if(quantize) process_uni_type_sync_shape_xy_quantize<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, skew_y, lfo_quantize);
  else process_uni_type_sync_shape_xy_quantize<GlobalUnison, Type, Sync>(block, modulation, shape, skew_x, skew_y, [](float in, int st) { return in; });
}

template <bool GlobalUnison, int Type, bool Sync, class Shape, class SkewX, class SkewY, class Quantize> void
lfo_engine::process_uni_type_sync_shape_xy_quantize(plugin_block& block, cv_cv_matrix_mixdown const* modulation, Shape shape, SkewX skew_x, SkewY skew_y, Quantize quantize)
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
    process_loop<GlobalUnison, Type, Sync>(block, modulation, processor, quantize);
  }
  else if (!x_is_exp && y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) { 
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, x, py, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync>(block, modulation, processor, quantize);
  }
  else if (x_is_exp && !y_is_exp)
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      return wave_calc_uni(in, px, y, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync>(block, modulation, processor, quantize);
  }
  else
  {
    auto processor = [skew_x, skew_y, shape](float in, float x, float y) {
      float px = std::log(0.001 + (x * 0.999)) / log_half;
      float py = std::log(0.001 + (y * 0.999)) / log_half;
      return wave_calc_uni(in, px, py, shape, skew_x, skew_y); };
    process_loop<GlobalUnison, Type, Sync>(block, modulation, processor, quantize);
  }
}

template <bool GlobalUnison, int Type, bool Sync, class Calc, class Quantize>
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

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == lfo_stage::end)
    {
      block.state.own_cv[0][0][f] = _filter_end_value;
      continue;
    }

    if (_graph_phase < 1.0f)
      _graph_phase += rate_curve[f] / block.sample_rate;

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

    // dont set the resample flag when graphing because that causes the lfo plot
    // for free-running stuff to get actually free-running when used in the cv plot.
    // we don't want that, the current lfo shape should be repeated and updated whenever
    // the free-running lfo "cycled"
    if (ref_wrapped && !block.graph)
    {
      _need_new_phase_for_graph = true;
      _new_ref_phase_for_graph = _ref_phase;
      if (is_noise_free_running(shape))
      {
        if(is_noise_static(shape))
          _seed_resample_table_for_graph = _static_noise.sample_table(true);
        else
          _seed_resample_table_for_graph = _smooth_noise.sample_table(true);
      }
      else if (is_noise_voice_rand(shape))
      {
        if (is_noise_static(shape))
          _seed_resample_table_for_graph = _static_noise.state();
        else
          _seed_resample_table_for_graph = _smooth_noise.state();
      }
    }

    bool ended = ref_wrapped && Type == type_one_shot || phase_wrapped && Type == type_one_phase;
    if (ended)
    {
      _stage = lfo_stage::filter;
      float filter_ms = block_auto[param_filter][0].real();
      _end_filter_stage_samples = block.sample_rate * filter_ms * 0.001;
    }
  }
}

}