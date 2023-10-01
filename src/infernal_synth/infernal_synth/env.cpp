#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/engine.hpp>
#include <plugin_base/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum class env_stage { a, d, s, r, end };

class env_engine: 
public module_engine {
  int const _slot;
  double _stage_pos = 0;
  env_stage _stage = {};
  double _release_level = 0;

public:
  INF_DECLARE_MOVE_ONLY(env_engine);
  env_engine(int slot) : _slot(slot) { initialize(); }
  void process(process_block& block) override;
  void initialize() override { _release_level = 0; _stage_pos = 0; _stage = env_stage::a; }
};

module_topo
env_topo()
{
  module_topo result(make_module(
    "{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Voice Env", 2, 
    module_stage::voice, module_output::cv, 1,
    gui_layout::tabbed, gui_position { 1, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int slot, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<env_engine>(slot); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { 1, 4 }));
  result.params.emplace_back(param_log(
    "{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", 1, section_main, 0, 10, 0.03, 1, 3, "Sec",
    param_dir::input, param_rate::accurate, param_format::plain, param_edit::knob,
    param_label_contents::both, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  result.params.emplace_back(param_log(
    "{45E37229-839F-4735-A31D-07DE9873DF04}", "D", 1, section_main, 0, 10, 0.1, 1, 3, "Sec",
    param_dir::input, param_rate::accurate, param_format::plain, param_edit::knob,
    param_label_contents::both, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_pct(
    "{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", 1, section_main, 0, 1, 0.5, 0,
    param_dir::input, param_rate::accurate, param_format::plain, true, param_edit::knob,
    param_label_contents::both, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position{ 0, 2 }));
  result.params.emplace_back(param_log(
    "{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", 1, section_main, 0, 10, 0.2, 1, 3, "Sec",
    param_dir::input, param_rate::accurate, param_format::plain, param_edit::knob,
    param_label_contents::both, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 3 }));

  return result;
}

void
env_engine::process(process_block& block)
{
  auto const& a = block.accurate_automation[env_param_a][0];
  auto const& d = block.accurate_automation[env_param_d][0];
  auto const& s = block.accurate_automation[env_param_s][0];
  auto const& r = block.accurate_automation[env_param_r][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == env_stage::end)
    {
      block.cv_out[f] = _release_level = 0;
      continue;
    }

    if (block.voice->state.release_frame == f)
    {
      _stage_pos = 0;
      _stage = env_stage::r;
    }

    if (_stage == env_stage::s)
    {
      block.cv_out[f] = _release_level = s[f];
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
      block.cv_out[f] = _release_level;
    else 
      switch (_stage)
      {
      case env_stage::a: block.cv_out[f] = _release_level = _stage_pos / stage_seconds; break;
      case env_stage::d: block.cv_out[f] = _release_level = 1.0 - _stage_pos / stage_seconds * (1.0 - s[f]); break;
      case env_stage::r: block.cv_out[f] = (1.0 - _stage_pos / stage_seconds) * _release_level; break;
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
