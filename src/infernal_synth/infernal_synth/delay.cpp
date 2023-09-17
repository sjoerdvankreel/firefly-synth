#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>

#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

class delay_engine: 
public module_engine {
public:
  INF_DECLARE_MOVE_ONLY(delay_engine);
  void process(
    plugin_topo const& topo, 
    plugin_block const& plugin, 
    module_block& module) override;
};

enum { section_main };
enum { param_on, param_out_gain };

module_topo
delay_topo()
{
  module_topo result(make_module("{ADA77C05-5D2B-4AA0-B705-A5BE89C32F37}", "Delay", 1, 
    module_scope::global, module_output::none));
  result.sections.emplace_back(section_topo(section_main, "Main"));
  result.params.emplace_back(param_toggle("{A8638DE3-B574-4584-99A2-EC6AEE725839}", "On", 1, 
    section_main, param_dir::input, param_label::both, false));
  result.params.emplace_back(param_pct("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Out Gain", 1,
    section_main, param_dir::output, param_edit::text, param_label::both, param_rate::block, true, 0, 1, 0));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<delay_engine>(); };
  return result;
}

void
delay_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  if (module.in.block()[param_on][0].step() == 0) return;

  float max_out = 0.0f;
  int voice_count = 2; // TODO
  for(int v = 0; v < voice_count; v++)
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < plugin.host->frame_count; f++)
      {
        plugin.host->audio_out[c][f] += plugin.out.voices[v][c][f];
        max_out = std::max(max_out, plugin.host->audio_out[c][f]);
      }

  auto const& param = topo.modules[module_filter].params[param_out_gain];
  plain_value out_gain = param.raw_to_plain(std::clamp(std::abs(max_out), 0.0f, 1.0f));
  module.out.params()[param_out_gain][0] = out_gain;
}

}
