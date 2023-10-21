#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum class env_stage { a, d, s, r, end };
enum { param_on, param_a, param_d, param_s, param_r };

class env_engine: 
public module_engine {
  double _stage_pos = 0;
  env_stage _stage = {};
  double _release_level = 0;

public:
  env_engine() { initialize(); }
  INF_PREVENT_ACCIDENTAL_COPY(env_engine);
  void process(plugin_block& block) override;
  void initialize() override { _release_level = 0; _stage_pos = 0; _stage = env_stage::a; }
};

module_topo
env_topo(plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Env", module_env, 3), 
    make_module_dsp(module_stage::voice, module_output::cv, 1, 0),
    make_module_gui(gui_layout::tabbed, pos, { 1, 1 })));

  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{2764871C-8E30-4780-B804-9E0FDE1A63EE}", "Main"),
    make_section_gui({ 0, 0 }, { 1, 5 })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{5EB485ED-6A5B-4A91-91F9-15BDEC48E5E6}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
      
  result.params.emplace_back(make_param(
    make_topo_info("{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", param_a, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.03, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{45E37229-839F-4735-A31D-07DE9873DF04}", "D", param_d, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", param_s, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", param_r, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.2, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<env_engine>(); };

  return result;
}

void
env_engine::process(plugin_block& block)
{
  if (block.state.own_block_automation[param_on][0].step() == 0 && block.module_slot != 0) return;

  auto const& a_curve = block.state.own_accurate_automation[param_a][0];
  auto const& d_curve = block.state.own_accurate_automation[param_d][0];
  auto const& s_curve = block.state.own_accurate_automation[param_s][0];
  auto const& r_curve = block.state.own_accurate_automation[param_r][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == env_stage::end)
    {
      block.state.own_cv[0][f] = _release_level = 0;
      continue;
    }

    if (block.voice->state.release_frame == f)
    {
      _stage_pos = 0;
      _stage = env_stage::r;
    }

    if (_stage == env_stage::s)
    {
      block.state.own_cv[0][f] = _release_level = s_curve[f];
      continue;
    }

    double stage_seconds;
    switch (_stage)
    {
    case env_stage::a: stage_seconds = block.normalized_to_raw(module_env, param_a, a_curve[f]); break;
    case env_stage::d: stage_seconds = block.normalized_to_raw(module_env, param_d, d_curve[f]); break;
    case env_stage::r: stage_seconds = block.normalized_to_raw(module_env, param_r, r_curve[f]); break;
    default: assert(false); stage_seconds = 0; break;
    }

    if(_stage_pos > stage_seconds) _stage_pos = stage_seconds;
    if (stage_seconds == 0)
      block.state.own_cv[0][f] = _release_level;
    else 
      switch (_stage)
      {
      case env_stage::a: block.state.own_cv[0][f] = _release_level = _stage_pos / stage_seconds; break;
      case env_stage::d: block.state.own_cv[0][f] = _release_level = 1.0 - _stage_pos / stage_seconds * (1.0 - s_curve[f]); break;
      case env_stage::r: block.state.own_cv[0][f] = (1.0 - _stage_pos / stage_seconds) * _release_level; break;
      default: assert(false); stage_seconds = 0; break;
      }

    check_unipolar(_release_level);
    _stage_pos += 1.0 / block.sample_rate;
    if (_stage_pos >= stage_seconds)
    {
      _stage_pos = 0;
      switch (_stage)
      {
      case env_stage::a: 
        _stage = env_stage::d; 
        break;
      case env_stage::d: 
        _stage = env_stage::s; 
        break;
      case env_stage::r: 
        _stage = env_stage::end; 
        if(block.module_slot == 0)
          block.voice->finished = true; 
        break;
      default: assert(false); break;
      }
    }
  }
}

}
