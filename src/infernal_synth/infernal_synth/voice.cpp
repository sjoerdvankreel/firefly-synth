#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

class voice_engine :
public module_engine {

public:
  void initialize() override { }
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_engine);
};

module_topo
voice_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{7110215B-A6C2-47F2-8A2F-DECA85250DF1}", "Voice", module_voice, 1),
    make_module_dsp(module_stage::voice, module_output::none, 0, 0), make_module_gui_none(section)));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<voice_engine> { return std::make_unique<voice_engine>(); };

  return result;
}

void
voice_engine::process(plugin_block& block)
{
  auto& mixer = get_audio_matrix_mixer(block, false);
  auto const& audio_in = mixer.mix(block, module_voice, 0);
  for(int c = 0; c < 2; c++) 
    audio_in[c].copy_to(block.start_frame, block.end_frame, block.voice->result[c]);
}

}
