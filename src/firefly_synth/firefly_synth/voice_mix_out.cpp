#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

class voice_mix_out_engine :
public module_engine {
public:
  void initialize() override { }
  void process(plugin_block& block) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_mix_out_engine);
};

module_topo
voice_topo(int section, bool out)
{
  module_topo result(make_module(
    make_topo_info("{94CC6FFA-2C0F-4B72-A484-65CD2974D288}", "Voice", module_voice_mix_out, 1), 
    make_module_dsp(module_stage::voice, module_output::none, 0, {}), 
    make_module_gui_none(section)));
  result.engine_factory = [](auto const&, int, int) -> std::unique_ptr<module_engine> { 
    return std::make_unique<voice_mix_out_engine>(); };
  return result;
}

void
voice_mix_out_engine::process(plugin_block& block)
{
  auto& mixer = get_audio_matrix_mixer(block, false);
  auto const& audio_in = mixer.mix(block, module_voice_mix_out, 0);
  for(int c = 0; c < 2; c++) 
    audio_in[c].copy_to(block.start_frame, block.end_frame, block.voice->result[c]);
}

}
