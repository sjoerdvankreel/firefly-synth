#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

static double const log_half = std::log(0.5);

enum { section_main };
enum class env_stage { delay, attack, hold, decay, sustain, release, end };
enum { param_on, param_delay, param_attack, param_attack_slope, param_hold, 
  param_decay, param_decay_slope, param_sustain, param_release, param_release_slope };

class env_engine: 
public module_engine {
  double _stage_pos = 0;
  env_stage _stage = {};
  double _release_level = 0;
  int current_stage_param() const;

public:
  void reset(plugin_block const*) override;
  void process(plugin_block& block) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(env_engine);
};

static void
init_default(plugin_state& state)
{ state.set_text_at(module_env, 1, param_on, 0, "On"); }

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  if (state.get_plain_at(module_env, mapping.module_slot, param_on, 0).step() == 0) return {};
  float delay = state.get_plain_at(module_env, mapping.module_slot, param_delay, 0).real();
  float attack = state.get_plain_at(module_env, mapping.module_slot, param_attack, 0).real();
  float hold = state.get_plain_at(module_env, mapping.module_slot, param_hold, 0).real();
  float decay = state.get_plain_at(module_env, mapping.module_slot, param_decay, 0).real();
  float release = state.get_plain_at(module_env, mapping.module_slot, param_release, 0).real();
  float sustain = std::max((delay + attack + hold + decay + release) / 5, 0.01f);
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
    make_module_dsp(module_stage::voice, module_output::cv, 0, { 
      make_module_dsp_output(true, make_topo_info("{2CDB809A-17BF-4936-99A0-B90E1035CBE6}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.graph_renderer = render_graph;
  result.default_initializer = init_default;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<env_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{2764871C-8E30-4780-B804-9E0FDE1A63EE}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 1, 1, 1, 1, 1, 1, 1 } })));
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{5EB485ED-6A5B-4A91-91F9-15BDEC48E5E6}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 }, gui_label_contents::none, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  on.domain.default_selector = [](int s, int) { return s == 0 ? "On" : "Off"; };
  on.gui.bindings.enabled.bind_slot([](int slot) { return slot > 0; });
  on.dsp.automate_selector = [](int s) { return s > 0 ? param_automate::automate : param_automate::none; };
      
  auto& delay = result.params.emplace_back(make_param(
    make_topo_info("{E9EF839C-235D-4248-A4E1-FAD62089CC78}", "D", param_delay, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  delay.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& attack = result.params.emplace_back(make_param(
    make_topo_info("{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", param_attack, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.03, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  attack.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& attack_slope = result.params.emplace_back(make_param(
    make_topo_info("{7C2DBB68-164D-45A7-9940-AB96F05D1777}", "A Slope", param_attack_slope, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label_none())));
  attack_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& hold = result.params.emplace_back(make_param(
    make_topo_info("{66F6036E-E64A-422A-87E1-34E59BC93650}", "H", param_hold, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  hold.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& decay = result.params.emplace_back(make_param(
    make_topo_info("{45E37229-839F-4735-A31D-07DE9873DF04}", "D", param_decay, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 5 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  decay.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& decay_slope = result.params.emplace_back(make_param(
    make_topo_info("{416C46E4-53E6-445E-8D21-1BA714E44EB9}", "D Slope", param_decay_slope, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 6 }, gui_label_contents::value,
      make_label_none())));
  decay_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& sustain = result.params.emplace_back(make_param(
    make_topo_info("{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", param_sustain, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 7 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sustain.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& release = result.params.emplace_back(make_param(
    make_topo_info("{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", param_release, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.2, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 8 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  release.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& release_slope = result.params.emplace_back(make_param(
    make_topo_info("{11113DB9-583A-48EE-A99F-6C7ABB693951}", "R Slope", param_release_slope, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 9 }, gui_label_contents::value,
      make_label_none())));
  release_slope.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

void
env_engine::reset(plugin_block const*)
{
  _stage_pos = 0;
  _release_level = 0;
  _stage = env_stage::delay;
}

int
env_engine::current_stage_param() const
{
  switch (_stage)
  {
  case env_stage::hold: return param_hold;
  case env_stage::delay: return param_delay;
  case env_stage::decay: return param_decay;
  case env_stage::attack: return param_attack;
  case env_stage::release: return param_release;
  default: assert(false); return -1;
  }
}

static inline double
make_section_curve(double slope, double split_pos, double slope_pos)
{
  double const slope_min = 0.0001;
  double const slope_max = 0.9999;
  double const slope_range = slope_max - slope_min;

  double slope_exp;
  double slope_bounded = slope_min + slope_range * slope;
  double split_bounded = slope_min + slope_range * split_pos;
  if (slope_bounded < 0.5f) slope_exp = std::log(slope_bounded);
  else slope_exp = std::log(1.0f - slope_bounded);
  if (slope_pos < split_bounded) return std::pow(slope_pos / split_bounded, slope_exp / log_half) * split_bounded;
  return 1 - std::pow(1.0f - (slope_pos - split_bounded) / (1.0f - split_bounded), slope_exp / log_half) * (1 - split_bounded);
}

void
env_engine::process(plugin_block& block)
{
  double const slope_min = 0.0001;
  double const slope_max = 0.9999;
  double const slope_range = slope_max - slope_min;

  bool on = block.state.own_block_automation[param_on][0].step();
  if (_stage == env_stage::end || (!on && block.module_slot != 0))
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  auto const& acc_auto = block.state.own_accurate_automation;
  auto const& sustain_curve = acc_auto[param_sustain][0];
  auto const& decay_slope_curve = acc_auto[param_decay_slope][0];
  auto const& attack_slope_curve = acc_auto[param_attack_slope][0];
  auto const& release_slope_curve = acc_auto[param_release_slope][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == env_stage::end)
    {
      _release_level = 0;
      block.state.own_cv[0][0][f] = 0;
      continue;
    }

    if (block.voice->state.release_frame == f)
    {
      _stage_pos = 0;
      _stage = env_stage::release;
    }

    if (_stage == env_stage::sustain)
    {
      _release_level = sustain_curve[f];
      block.state.own_cv[0][0][f] = sustain_curve[f];
      continue;
    }

    int stage_param = current_stage_param();
    auto const& stage_curve = acc_auto[stage_param][0];
    double stage_seconds = block.normalized_to_raw(module_env, stage_param, stage_curve[f]);
    _stage_pos = std::min(_stage_pos, stage_seconds);

    float out = 0;
    double slope_exp = 0;
    double split_pos = 0;
    double slope_bounded = 0;
    double slope_pos = _stage_pos / stage_seconds;
    
    if (stage_seconds == 0)
      out = _release_level;
    else switch (_stage)
    {
    case env_stage::hold: out = 1; _release_level = 1; break;
    case env_stage::delay: out = 0; _release_level = 0; break;
    case env_stage::attack:
      out = make_section_curve(attack_slope_curve[f], 1.0f - attack_slope_curve[f], _stage_pos / stage_seconds);
      _release_level = out;
      break;
    case env_stage::decay:
      slope_bounded = slope_min + decay_slope_curve[f] * slope_range;
      split_pos = slope_bounded;
      if (decay_slope_curve[f] < 0.5f) slope_exp = std::log(slope_bounded);
      else slope_exp = std::log(1.0f - slope_bounded);
      if (slope_pos < split_pos) out = 1 - (std::pow(slope_pos / split_pos, slope_exp / log_half) * split_pos);
      else out = std::pow(1.0f - (slope_pos - split_pos) / (1.0f - split_pos), slope_exp / log_half) * (1 - split_pos);
      _release_level = out;
      break;
    case env_stage::release:
      slope_exp = std::log(slope_min + release_slope_curve[f] * slope_range);
      out = (1.0 - std::pow(_stage_pos / stage_seconds, slope_exp / log_half)) * _release_level;
     break;
    default: 
      assert(false); 
      stage_seconds = 0; 
      break;
    }
    check_unipolar(out);
    block.state.own_cv[0][0][f] = out;

    check_unipolar(_release_level);
    _stage_pos += 1.0 / block.sample_rate;
    if(_stage_pos < stage_seconds) continue;

    _stage_pos = 0;
    switch (_stage)
    {
    case env_stage::hold: _stage = env_stage::decay; break;
    case env_stage::attack: _stage = env_stage::hold; break;
    case env_stage::delay: _stage = env_stage::attack; break;
    case env_stage::decay: _stage = env_stage::sustain; break;
    case env_stage::release: _stage = env_stage::end; block.voice->finished |= block.module_slot == 0; break;
    default: assert(false); break;
    }
  }
}

}
