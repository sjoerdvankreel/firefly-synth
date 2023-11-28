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
enum class env_stage { a, d, s, r, end };
enum { param_on, param_a, param_d, param_s, param_r };

class env_engine: 
public module_engine {
  double _stage_pos = 0;
  env_stage _stage = {};
  double _release_level = 0;
  int current_stage_param() const;

public:
  env_engine() { initialize(); }
  PB_PREVENT_ACCIDENTAL_COPY(env_engine);
  void process(plugin_block& block) override;
  void initialize() override { _release_level = 0; _stage_pos = 0; _stage = env_stage::a; }
};

static void
init_default(plugin_state& state)
{
  state.set_text_at(module_env, 1, param_on, 0, "On");
}

static graph_data
render_graph(plugin_state const& state, int slot)
{
  if (state.get_plain_at(module_env, slot, param_on, 0).step() == 0) return {};

  float a = state.get_plain_at(module_env, slot, param_a, 0).real();
  float d = state.get_plain_at(module_env, slot, param_d, 0).real();
  float r = state.get_plain_at(module_env, slot, param_r, 0).real();
  float s = std::max((a + d + r) / 3, 0.01f);
  float ads = a + d + s;
  float adsr = ads + r;

  module_graph_params params = {};
  params.bpm = 120;
  params.frame_count = 200;
  params.module_slot = slot;
  params.module_index = module_env;
  params.sample_rate = params.frame_count / adsr;
  params.voice_release_at = ads / adsr * params.frame_count;

  module_graph_engine engine(&state, params);
  return engine.render([](plugin_block& block) {
    env_engine engine;
    engine.initialize();
    engine.process(block);
    graph_data result;
    result.series = true;
    result.bipolar = false;
    result.series_data = block.state.own_cv[0][0].data();
    result.series_data.push_back(0);
    return result;
  });
}

module_topo
env_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Env", module_env, 3), 
    make_module_dsp(module_stage::voice, module_output::cv, 0, { 
      make_module_dsp_output(true, make_topo_info("{2CDB809A-17BF-4936-99A0-B90E1035CBE6}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.graph_renderer = render_graph;
  result.default_initializer = init_default;
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<env_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{2764871C-8E30-4780-B804-9E0FDE1A63EE}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 1, 1 } })));
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{5EB485ED-6A5B-4A91-91F9-15BDEC48E5E6}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  on.domain.default_selector = [](int s, int) { return s == 0 ? "On" : "Off"; };
  on.gui.bindings.enabled.bind_slot([](int slot) { return slot > 0; });
  on.dsp.automate_selector = [](int s) { return s > 0 ? param_automate::automate : param_automate::none; };
      
  auto& a = result.params.emplace_back(make_param(
    make_topo_info("{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", param_a, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.03, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  a.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& d = result.params.emplace_back(make_param(
    make_topo_info("{45E37229-839F-4735-A31D-07DE9873DF04}", "D", param_d, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_log(0, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  d.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& s = result.params.emplace_back(make_param(
    make_topo_info("{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", param_s, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  s.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& r = result.params.emplace_back(make_param(
    make_topo_info("{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", param_r, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(0, 10, 0.2, 1, 3, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  r.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

int
env_engine::current_stage_param() const
{
  switch (_stage)
  {
  case env_stage::a: return param_a;
  case env_stage::d: return param_d;
  case env_stage::r: return param_r;
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
  auto const& s_curve = acc_auto[param_s][0];

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
      _stage = env_stage::r;
    }

    if (_stage == env_stage::s)
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
      case env_stage::a: 
        out = _stage_pos / stage_seconds; 
        _release_level = out;
        break;
      case env_stage::d: 
        out = 1.0 - _stage_pos / stage_seconds * (1.0 - s_curve[f]); 
        _release_level = out;
        break;
      case env_stage::r: 
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
    case env_stage::a: _stage = env_stage::d; break;
    case env_stage::d: _stage = env_stage::s; break;
    case env_stage::r: 
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
