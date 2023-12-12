#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

class voice_mix_engine :
public module_engine {
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const*) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_mix_engine);
};

module_topo
voice_mix_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{70C5721B-4D0C-4ED3-B5B9-3D3E0D46C62E}", "Voice Mix", "V.Mix", true, module_voice_mix, 1),
    make_module_dsp(module_stage::output, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{FFA367C9-23C1-4E89-95C5-90EE59CB034D}", "Output", 0, 1)) }), 
    make_module_gui_none(section)));
  
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<voice_mix_engine>(); };
  return result;
}

void
voice_mix_engine::process(plugin_block& block)
{
  for(int c = 0; c < 2; c++)
    block.out->voice_mixdown[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][c]);
}

}
