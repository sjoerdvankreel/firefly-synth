#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { section_main };
enum class env_stage { delay, attack, hold, decay, sustain, release, end };
enum { param_on, param_delay, param_attack, param_hold, param_decay, param_sustain, param_release };

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
{
  state.set_text_at(module_env, 1, param_on, 0, "On");
}

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  // TODO DAHDR
  if (state.get_plain_at(module_env, mapping.module_slot, param_on, 0).step() == 0) return {};

  float attack = state.get_plain_at(module_env, mapping.module_slot, param_attack, 0).real();
  float decay = state.get_plain_at(module_env, mapping.module_slot, param_decay, 0).real();
  float release = state.get_plain_at(module_env, mapping.module_slot, param_release, 0).real();
  float sustain = std::max((attack + decay + release) / 3, 0.01f);
  float ads = attack + decay + sustain;
  float adsr = ads + release;

  graph_engine_params params = {};
  params.bpm = 120;
  params.frame_count = 200;
  params.sample_rate = params.frame_count / adsr;
  params.voice_release_at = ads / adsr * params.frame_count;

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
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 1, 1, 1, 1 } })));
  
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
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  delay.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& attack = result.params.emplace_back(make_param(
    make_topo_info("{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", param_attack, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.03, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  attack.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& hold = result.params.emplace_back(make_param(
    make_topo_info("{66F6036E-E64A-422A-87E1-34E59BC93650}", "H", param_hold, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  hold.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& decay = result.params.emplace_back(make_param(
    make_topo_info("{45E37229-839F-4735-A31D-07DE9873DF04}", "D", param_decay, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  decay.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& sustain = result.params.emplace_back(make_param(
    make_topo_info("{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", param_sustain, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 5 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sustain.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& release = result.params.emplace_back(make_param(
    make_topo_info("{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", param_release, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.2, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 6 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  release.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

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
  case env_stage::delay: return param_delay;
  case env_stage::attack: return param_attack;
  case env_stage::hold: return param_hold;
  case env_stage::decay: return param_decay;
  case env_stage::release: return param_release;
  default: assert(false); return -1;
  }
}

void
env_engine::process(plugin_block& block)
{
  bool on = block.state.own_block_automation[param_on][0].step();
  if (_stage == env_stage::end || (!on && block.module_slot != 0))
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  auto const& acc_auto = block.state.own_accurate_automation;
  auto const& s_curve = acc_auto[param_sustain][0];

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
      _release_level = s_curve[f];
      block.state.own_cv[0][0][f] = s_curve[f];
      continue;
    }

    int stage_param = current_stage_param();
    auto const& stage_curve = acc_auto[stage_param][0];
    double stage_seconds = block.normalized_to_raw(module_env, stage_param, stage_curve[f]);
    _stage_pos = std::min(_stage_pos, stage_seconds);

    float out = 0;
    if (stage_seconds == 0)
      out = _release_level;
    else switch (_stage)
    {
      case env_stage::delay:
        out = 0;
        _release_level = 0;
        break;
      case env_stage::attack: 
        out = _stage_pos / stage_seconds; 
        _release_level = out;
        break;
      case env_stage::hold:
        out = 1;
        _release_level = 1;
        break;
      case env_stage::decay:
        out = 1.0 - _stage_pos / stage_seconds * (1.0 - s_curve[f]); 
        _release_level = out;
        break;
      case env_stage::release: 
        out = (1.0 - _stage_pos / stage_seconds) * _release_level; 
        break;
      default: 
        assert(false); 
        stage_seconds = 0; 
        break;
    }
    block.state.own_cv[0][0][f] = out;

    check_unipolar(_release_level);
    _stage_pos += 1.0 / block.sample_rate;
    if(_stage_pos < stage_seconds) continue;

    _stage_pos = 0;
    switch (_stage)
    {
    case env_stage::delay: _stage = env_stage::attack; break;
    case env_stage::attack: _stage = env_stage::hold; break;
    case env_stage::hold: _stage = env_stage::decay; break;
    case env_stage::decay: _stage = env_stage::sustain; break;
    case env_stage::release: 
      _stage = env_stage::end; 
      if(block.module_slot == 0)
        block.voice->finished = true; 
      break;
    default: 
      assert(false); 
      break;
    }
  }
}

}
