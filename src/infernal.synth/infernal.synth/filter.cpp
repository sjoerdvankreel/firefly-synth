#include <infernal.base/dsp.hpp>
#include <infernal.base/topo.hpp>
#include <infernal.base/support.hpp>
#include <infernal.base/engine.hpp>
#include <infernal.synth/synth.hpp>
#include <cmath>

using namespace infernal::base;

namespace infernal::synth {

class filter_engine: 
public module_engine {  
public:
  virtual void 
  process(plugin_topo const& topo, plugin_block const& plugin, module_block& module) override;
};

static std::vector<item_topo>
type_items()
{
  std::vector<item_topo> result;
  result.emplace_back("{FA311D7F-C813-4636-9BF8-102687EA5706}", "LPF");
  result.emplace_back("{6A837BF2-F7C2-4833-A897-3A26ED268831}", "HPF");
  return result;
}

enum filter_type { filter_type_lpf, filter_type_hpf };
enum filter_group { filter_group_main, filter_group_filter };
enum filter_param { filter_param_on, filter_param_type, filter_param_res, filter_param_freq };

module_group_topo
filter_topo()
{
  module_group_topo result(make_module_group("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", 1, module_scope::voice, module_output::audio));
  result.param_groups.emplace_back(param_group_topo(filter_group_main, "Main"));
  result.param_groups.emplace_back(param_group_topo(filter_group_filter, "Filter"));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };
  result.params.emplace_back(make_param_toggle("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", "Off", filter_group_main, config_input_toggle()));
  result.params.emplace_back(make_param_list("{23C095D3-BB7A-41AB-B944-2969B43DFD5C}", "Type", "LPF", type_items(), filter_group_main, config_input_list_list()));
  result.params.emplace_back(make_param_pct("{321F7118-B62E-420F-9790-9997AE9A4B82}", "Res", "0", 0, 1, filter_group_filter, config_input_linear_accurate_knob()));
  result.params.emplace_back(make_param_log("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", "1000", "Hz", 20, 20000, 1000, filter_group_filter, config_input_log_accurate_knob()));
  return result;
}

void
filter_engine::process(plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  auto const& osc_audio = plugin.module_audio[module_type::module_type_osc];
  for(int o = 0; o < topo.module_groups[module_type_osc].module_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < plugin.host->frame_count; f++)
        module.audio_output[c][f] += osc_audio[o][c][f];
}

}
