#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum class env_stage { delay, attack, hold, decay, sustain, release, end };

enum { type_sustain, type_follow, type_release };
enum { section_main, section_slope, section_dhadsr };
enum { scratch_delay, scratch_attack, scratch_hold, scratch_decay, scratch_release, scratch_count };
enum { 
  param_on, param_type, param_sync, param_multi,
  param_attack_slope, param_decay_slope, param_release_slope,
  param_delay_time, param_delay_tempo, param_attack_time, param_attack_tempo,
  param_hold_time, param_hold_tempo, param_decay_time, param_decay_tempo, 
  param_sustain, param_release_time, param_release_tempo };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{021EA627-F467-4879-A045-3694585AD694}", "Sustain");
  result.emplace_back("{927DBB76-A0F2-4007-BD79-B205A3697F31}", "Follow");
  result.emplace_back("{0AF743E3-9248-4FF6-98F1-0847BD5790FA}", "Release");
  return result;
}

class env_engine: 
public module_engine {

public:
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(env_engine);
  void process(plugin_block& block) override;
  void reset(plugin_block const* block) override;

private:
  double _att_exp = 0;
  double _dcy_exp = 0;
  double _rls_exp = 0;
  double _att_splt_bnd = 0;
  double _dcy_splt_bnd = 0;
  double _rls_splt_bnd = 0;

  env_stage _stage = {};
  double _stage_pos = 0;
  double _release_level = 0;

  static inline double const slope_min = 0.0001;
  static inline double const slope_max = 0.9999;
  static inline double const slope_range = slope_max - slope_min;

  double section_slope(double slope_pos, double splt_bnd, double exp);
  void init_section(double slope, double split_pos, double& exp, double& splt_bnd);
};

static void
init_default(plugin_state& state)
{ state.set_text_at(module_env, 1, param_on, 0, "On"); }

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  if (state.get_plain_at(module_env, mapping.module_slot, param_on, 0).step() == 0) return {};

  float const bpm = 120;
  bool sync = state.get_plain_at(module_env, mapping.module_slot, param_sync, 0).step() != 0;
  bool do_sustain = state.get_plain_at(module_env, mapping.module_slot, param_type, 0).step() == type_sustain;
  float hold = sync_or_time_from_state(state, bpm, sync, module_env, mapping.module_slot, param_hold_time, param_hold_tempo);
  float delay = sync_or_time_from_state(state, bpm, sync, module_env, mapping.module_slot, param_delay_time, param_delay_tempo);
  float decay = sync_or_time_from_state(state, bpm, sync, module_env, mapping.module_slot, param_decay_time, param_decay_tempo);
  float attack = sync_or_time_from_state(state, bpm, sync, module_env, mapping.module_slot, param_attack_time, param_attack_tempo);
  float release = sync_or_time_from_state(state, bpm, sync, module_env, mapping.module_slot, param_release_time, param_release_tempo);

  float sustain = !do_sustain ? 0.0f: std::max((delay + attack + hold + decay + release) / 5, 0.01f);
  float dahds = delay + attack + hold + decay + sustain;
  float dahdsr = dahds + release;

  graph_engine_params params = {};
  params.bpm = 120;
  params.frame_count = 200;
  params.sample_rate = params.frame_count / dahdsr;
  params.voice_release_at = dahds / dahdsr * params.frame_count;

  graph_engine engine(&state, params);
  auto const* block = engine.process_default(module_env, mapping.module_slot);
  jarray<float, 1> series(block->state.own_cv[0][0]);
  series.push_back(0);
  return graph_data(series, false);
}

module_topo
env_topo(int section, gui_colors const& colors, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Envelope", "Env", true, module_env, 4),
    make_module_dsp(module_stage::voice, module_output::cv, scratch_count, { 
      make_module_dsp_output(true, make_topo_info("{2CDB809A-17BF-4936-99A0-B90E1035CBE6}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1, 1 }, { 7, 6 } })));

  result.graph_renderer = render_graph;
  result.default_initializer = init_default;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<env_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{2764871C-8E30-4780-B804-9E0FDE1A63EE}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 1 } })));

  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{5EB485ED-6A5B-4A91-91F9-15BDEC48E5E6}", "On", param_on, 1),
    make_param_dsp_voice(param_automate::none), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 }, gui_label_contents::none,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  on.domain.default_selector = [](int s, int) { return s == 0 ? "On" : "Off"; };
  on.gui.bindings.enabled.bind_slot([](int slot) { return slot > 0; });

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{E6025B4A-495C-421F-9A9A-8D2A247F94E7}", "Type", param_type, 1),
    make_param_dsp_voice(param_automate::none), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 }, gui_label_contents::name,
      make_label_none())));
  type.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{4E2B3213-8BCF-4F93-92C7-FA59A88D5B3C}", "Tempo Sync", "Sync", true, param_sync, 1),
    make_param_dsp_voice(param_automate::none), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 2 }, gui_label_contents::name, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  sync.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& multi = result.params.emplace_back(make_param(
    make_topo_info("{84B6DC4D-D2FF-42B0-992D-49B561C46013}", "Multi Trigger", "Multi", true, param_multi, 1),
    make_param_dsp_voice(param_automate::none), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 3 }, gui_label_contents::name,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  multi.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  result.sections.emplace_back(make_param_section(section_slope,
    make_topo_tag("{9297FA9D-1C0B-4290-AC5F-BC63D38A40D4}", "Slope"),
    make_param_section_gui({ 0, 1 }, { 1, 3 })));

  auto& attack_slope = result.params.emplace_back(make_param(
    make_topo_info("{7C2DBB68-164D-45A7-9940-AB96F05D1777}", "Attack Slope", "A.Slp", true, param_attack_slope, 1),
    make_param_dsp_voice(param_automate::none), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_slope, gui_edit_type::knob, { 0, 0 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  attack_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  auto& decay_slope = result.params.emplace_back(make_param(
    make_topo_info("{416C46E4-53E6-445E-8D21-1BA714E44EB9}", "Decay Slope", "D.Slp", true, param_decay_slope, 1),
    make_param_dsp_voice(param_automate::none), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_slope, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  decay_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  auto& release_slope = result.params.emplace_back(make_param(
    make_topo_info("{11113DB9-583A-48EE-A99F-6C7ABB693951}", "Release Slope", "R.Slp", true, param_release_slope, 1),
    make_param_dsp_voice(param_automate::none), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_slope, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  release_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  result.sections.emplace_back(make_param_section(section_dhadsr,
    make_topo_tag("{96BDC7C2-7DF4-4CC5-88F9-2256975D70AC}", "DAHDSR"),
    make_param_section_gui({ 1, 0, 1, 2 }, { 1, 6 })));
      
  auto& delay_time = result.params.emplace_back(make_param(
    make_topo_info("{E9EF839C-235D-4248-A4E1-FAD62089CC78}", "Delay Time", "Dly", true, param_delay_time, 1),
    make_param_dsp_voice(param_automate::none), make_domain_log(0, 10, 0, 1, 3, "Sec"),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 0 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_time.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  delay_time.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] == 0; });
  auto& delay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{A016A3B5-8BFC-4DCD-B41F-F69F3A239AFA}", "Delay Tempo", "Dly", true, param_delay_tempo, 1),
    make_param_dsp_voice(param_automate::none), make_domain_timesig_default(true, { 0, 1 }),
    make_param_gui_single(section_dhadsr, gui_edit_type::list, { 0, 0 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_tempo.gui.submenu = make_timesig_submenu(delay_tempo.domain.timesigs);
  delay_tempo.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  delay_tempo.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] != 0; });

  auto& attack_time = result.params.emplace_back(make_param(
    make_topo_info("{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "Attack Time", "A", true, param_attack_time, 1),
    make_param_dsp_voice(param_automate::none), make_domain_log(0, 10, 0.03, 1, 3, "Sec"),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  attack_time.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  attack_time.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] == 0; });
  auto& attack_tempo = result.params.emplace_back(make_param(
    make_topo_info("{3130A19C-AA2C-40C8-B586-F3A1E96ED8C6}", "Attack Tempo", "A", true, param_attack_tempo, 1),
    make_param_dsp_voice(param_automate::none), make_domain_timesig_default(true, { 1, 64 }),
    make_param_gui_single(section_dhadsr, gui_edit_type::list, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  attack_tempo.gui.submenu = make_timesig_submenu(attack_tempo.domain.timesigs);
  attack_tempo.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  attack_tempo.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] != 0; });

  auto& hold_time = result.params.emplace_back(make_param(
    make_topo_info("{66F6036E-E64A-422A-87E1-34E59BC93650}", "Hold Time", "Hld", true, param_hold_time, 1),
    make_param_dsp_voice(param_automate::none), make_domain_log(0, 10, 0, 1, 3, "Sec"),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  hold_time.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  hold_time.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] == 0; });
  auto& hold_tempo = result.params.emplace_back(make_param(
    make_topo_info("{97846CDB-7349-4DE9-8BDF-14EAD0586B28}", "Hold Tempo", "Hld", true, param_hold_tempo, 1),
    make_param_dsp_voice(param_automate::none), make_domain_timesig_default(true, { 0, 1}),
    make_param_gui_single(section_dhadsr, gui_edit_type::list, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  hold_tempo.gui.submenu = make_timesig_submenu(hold_tempo.domain.timesigs);
  hold_tempo.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  hold_tempo.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] != 0; });

  auto& decay_time = result.params.emplace_back(make_param(
    make_topo_info("{45E37229-839F-4735-A31D-07DE9873DF04}", "Decay Time", "D", true, param_decay_time, 1),
    make_param_dsp_voice(param_automate::none), make_domain_log(0, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  decay_time.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  decay_time.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] == 0; });
  auto& decay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{47253C57-FBCA-4A49-AF88-88AC9F4781D7}", "Decay Tempo", "D", true, param_decay_tempo, 1),
    make_param_dsp_voice(param_automate::none), make_domain_timesig_default(true, { 1, 32 }),
    make_param_gui_single(section_dhadsr, gui_edit_type::list, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  decay_tempo.gui.submenu = make_timesig_submenu(decay_tempo.domain.timesigs);
  decay_tempo.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  decay_tempo.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] != 0; });

  auto& sustain = result.params.emplace_back(make_param(
    make_topo_info("{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "Sustain", "Stn", true, param_sustain, 1),
    make_param_dsp_voice(param_automate::none), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  sustain.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& release_time = result.params.emplace_back(make_param(
    make_topo_info("{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "Release Time", "R", true, param_release_time, 1),
    make_param_dsp_voice(param_automate::none), make_domain_log(0, 10, 0.2, 1, 3, "Sec"),
    make_param_gui_single(section_dhadsr, gui_edit_type::hslider, { 0, 5 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  release_time.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  release_time.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] == 0; });
  auto& release_tempo = result.params.emplace_back(make_param(
    make_topo_info("{FDC00AA5-8648-4064-BE77-1A9CDB6B53EE}", "Release Tempo", "R", true, param_release_tempo, 1),
    make_param_dsp_voice(param_automate::none), make_domain_timesig_default(true, {1, 16 }),
    make_param_gui_single(section_dhadsr, gui_edit_type::list, { 0, 5 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  release_tempo.gui.submenu = make_timesig_submenu(release_tempo.domain.timesigs);
  release_tempo.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  release_tempo.gui.bindings.visible.bind_params({ param_sync }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

void 
env_engine::reset(plugin_block const* block)
{
  _stage_pos = 0; 
  _release_level = 0; 
  _stage = env_stage::delay;
  auto const& block_auto = block->state.own_block_automation;
  float ds = block_auto[param_decay_slope][0].real();
  float as = block_auto[param_attack_slope][0].real();
  float rs = block_auto[param_release_slope][0].real();
  init_section(ds, ds, _dcy_exp, _dcy_splt_bnd);
  init_section(rs, rs, _rls_exp, _rls_splt_bnd);
  init_section(as, 1 - as, _att_exp, _att_splt_bnd);
}

void 
env_engine::init_section(double slope, double split_pos, double& exp, double& splt_bnd)
{
  splt_bnd = slope_min + slope_range * split_pos;
  double slope_bounded = slope_min + slope_range * slope;
  if (slope_bounded < 0.5f) exp = std::log(slope_bounded) / std::log(0.5);
  else exp = std::log(1.0f - slope_bounded) / std::log(0.5);
}

double 
env_engine::section_slope(double slope_pos, double splt_bnd, double exp)
{ 
  if (slope_pos < splt_bnd)
    return std::pow(slope_pos / splt_bnd, exp) * splt_bnd;
  return 1 - std::pow(1 - (slope_pos - splt_bnd) / (1 - splt_bnd), exp) * (1 - splt_bnd);
}

void
env_engine::process(plugin_block& block)
{
  auto const& block_auto = block.state.own_block_automation;
  bool on = block_auto[param_on][0].step() != 0;
  if (_stage == env_stage::end || (!on && block.module_slot != 0))
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  int type = block_auto[param_type][0].step();
  bool sync = block_auto[param_sync][0].step() != 0;
  float stn = block_auto[param_sustain][0].real();
  float hld = block_auto[param_hold_time][0].real();
  float dcy = block_auto[param_decay_time][0].real();
  float dly = block_auto[param_delay_time][0].real();
  float att = block_auto[param_attack_time][0].real();
  float rls = block_auto[param_release_time][0].real();
  if (sync)
  {
    auto const& params = block.plugin.modules[module_env].params;
    hld = timesig_to_time(block.host.bpm, params[param_hold_tempo].domain.timesigs[block_auto[param_hold_tempo][0].step()]);
    dcy = timesig_to_time(block.host.bpm, params[param_decay_tempo].domain.timesigs[block_auto[param_decay_tempo][0].step()]);
    dly = timesig_to_time(block.host.bpm, params[param_delay_tempo].domain.timesigs[block_auto[param_delay_tempo][0].step()]);
    att = timesig_to_time(block.host.bpm, params[param_attack_tempo].domain.timesigs[block_auto[param_attack_tempo][0].step()]);
    rls = timesig_to_time(block.host.bpm, params[param_release_tempo].domain.timesigs[block_auto[param_release_tempo][0].step()]);
  }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == env_stage::end)
    {
      _release_level = 0;
      block.state.own_cv[0][0][f] = 0;
      continue;
    }

    if (block.voice->state.release_frame == f && (type == type_sustain || 
      type == type_release && _stage != env_stage::release))
    {
      _stage_pos = 0;
      _stage = env_stage::release;
    }

    if (_stage == env_stage::sustain && type == type_sustain)
    {
      _release_level = stn;
      block.state.own_cv[0][0][f] = stn;
      continue;
    }

    double stage_seconds = 0;
    switch (_stage)
    {
    case env_stage::hold: stage_seconds = hld; break;
    case env_stage::decay: stage_seconds = dcy; break;
    case env_stage::delay: stage_seconds = dly; break;
    case env_stage::attack: stage_seconds = att; break;
    case env_stage::release: stage_seconds = rls; break;
    default: assert(false); break;
    }

    float out = 0;
    _stage_pos = std::min(_stage_pos, stage_seconds);
    double slope_pos = _stage_pos / stage_seconds;
    if (stage_seconds == 0) out = _release_level;
    else switch (_stage)
    {
    case env_stage::hold: _release_level = out = 1; break;
    case env_stage::delay: _release_level = out = 0; break;
    case env_stage::attack: _release_level = out = section_slope(slope_pos, _att_splt_bnd, _att_exp); break;
    case env_stage::release: out = _release_level * (1 - section_slope(slope_pos, _rls_splt_bnd, _rls_exp)); break;
    case env_stage::decay: _release_level = out = stn + (1 - stn) * (1 - section_slope(slope_pos, _dcy_splt_bnd, _dcy_exp)); break;
    default: assert(false); stage_seconds = 0; break;
    }
    
    check_unipolar(out);
    check_unipolar(_release_level);
    block.state.own_cv[0][0][f] = out;
    _stage_pos += 1.0 / block.sample_rate;
    if(_stage_pos < stage_seconds) continue;

    _stage_pos = 0;
    switch (_stage)
    {
    case env_stage::hold: _stage = env_stage::decay; break;
    case env_stage::attack: _stage = env_stage::hold; break;
    case env_stage::delay: _stage = env_stage::attack; break;
    case env_stage::decay: _stage = type == type_sustain? env_stage::sustain: env_stage::release; break;
    case env_stage::release: _stage = env_stage::end; block.voice->finished |= block.module_slot == 0; break;
    default: assert(false); break;
    }
  }
}

}
