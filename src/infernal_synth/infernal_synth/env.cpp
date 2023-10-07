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
enum { param_a, param_d, param_s, param_r };

class env_engine: 
public module_engine {
  int const _slot;
  double _stage_pos = 0;
  env_stage _stage = {};
  double _release_level = 0;

public:
  INF_PREVENT_ACCIDENTAL_COPY(env_engine);
  env_engine(int slot) : _slot(slot) { initialize(); }
  void process(plugin_block& block) override;
  void initialize() override { _release_level = 0; _stage_pos = 0; _stage = env_stage::a; }
};

module_topo
env_topo()
{
  module_topo result(make_module(
    make_topo_info("{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Voice Env", module_env, 2), 
    make_module_dsp(module_stage::voice, module_output::cv, 1),
    make_module_gui(gui_layout::tabbed, { 1, 0 }, { 1, 1 })));
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{2764871C-8E30-4780-B804-9E0FDE1A63EE}", "Main"),
    make_section_gui({ 0, 0 }, { 1, 4 })));
  result.engine_factory = [](int slot, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<env_engine>(slot); };
  
  result.params.emplace_back(param_log(
    "{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", param_a, 1, section_main, 0, 10, 0.03, 1, 3, "Sec",
    param_direction::input, param_rate::accurate, param_format::plain, gui_edit_type::knob,
    gui_label_contents::both, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  
  result.params.emplace_back(param_log(
    "{45E37229-839F-4735-A31D-07DE9873DF04}", "D", param_d, 1, section_main, 0, 10, 0.1, 1, 3, "Sec",
    param_direction::input, param_rate::accurate, param_format::plain, gui_edit_type::knob,
    gui_label_contents::both, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  
  result.params.emplace_back(param_pct(
    "{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", param_s, 1, section_main, 0, 1, 0.5, 0,
    param_direction::input, param_rate::accurate, param_format::plain, true, gui_edit_type::knob,
    gui_label_contents::both, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position{ 0, 2 }));
  
  result.params.emplace_back(param_log(
    "{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", param_r, 1, section_main, 0, 10, 0.2, 1, 3, "Sec",
    param_direction::input, param_rate::accurate, param_format::plain, gui_edit_type::knob,
    gui_label_contents::both, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position { 0, 3 }));

  return result;
}

void
env_engine::process(plugin_block& block)
{
  auto const& a = block.state.accurate_automation[param_a][0];
  auto const& d = block.state.accurate_automation[param_d][0];
  auto const& s = block.state.accurate_automation[param_s][0];
  auto const& r = block.state.accurate_automation[param_r][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == env_stage::end)
    {
      block.state.own_cv_out[0][f] = _release_level = 0;
      continue;
    }

    if (block.voice->state.release_frame == f)
    {
      _stage_pos = 0;
      _stage = env_stage::r;
    }

    if (_stage == env_stage::s)
    {
      block.state.own_cv_out[0][f] = _release_level = s[f];
      continue;
    }

    double stage_seconds;
    switch (_stage)
    {
    case env_stage::a: stage_seconds = a[f]; break;
    case env_stage::d: stage_seconds = d[f]; break;
    case env_stage::r: stage_seconds = r[f]; break;
    default: assert(false); stage_seconds = 0; break;
    }

    if(_stage_pos > stage_seconds) _stage_pos = stage_seconds;
    if (stage_seconds == 0)
      block.state.own_cv_out[0][f] = _release_level;
    else 
      switch (_stage)
      {
      case env_stage::a: block.state.own_cv_out[0][f] = _release_level = _stage_pos / stage_seconds; break;
      case env_stage::d: block.state.own_cv_out[0][f] = _release_level = 1.0 - _stage_pos / stage_seconds * (1.0 - s[f]); break;
      case env_stage::r: block.state.own_cv_out[0][f] = (1.0 - _stage_pos / stage_seconds) * _release_level; break;
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
        if(_slot == 0)
          block.voice->finished = true; 
        break;
      default: assert(false); break;
      }
    }
  }
}

}
