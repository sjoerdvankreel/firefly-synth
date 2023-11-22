#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

class voice_engine :
public module_engine {
  bool const _out;
public:
  voice_engine(bool out): _out(out) {}
  PB_PREVENT_ACCIDENTAL_COPY(voice_engine);
  void initialize() override { }
  void process(plugin_block& block) override;
};

module_topo
voice_topo(int section, bool out)
{
  auto const out_dsp = make_module_dsp(module_stage::voice, module_output::none, 0, {});
  auto const in_dsp = make_module_dsp(module_stage::output, module_output::audio, 0, {
    make_module_dsp_output(false, make_topo_info("{FFA367C9-23C1-4E89-95C5-90EE59CB034D}", "Output", 0, 1)) });
  auto const in_info = make_topo_info("{70C5721B-4D0C-4ED3-B5B9-3D3E0D46C62E}", "Voice", module_voice_in, 1);
  auto const out_info = make_topo_info("{94CC6FFA-2C0F-4B72-A484-65CD2974D288}", "Voice", module_voice_out, 1);
  auto dsp = module_dsp(out ? out_dsp : in_dsp);
  auto info = topo_info(out? out_info: in_info);
  module_topo result(make_module(info, dsp, make_module_gui_none(section)));
  result.engine_factory = [out](auto const&, int, int) ->
    std::unique_ptr<voice_engine> { return std::make_unique<voice_engine>(out); };
  return result;
}

void
voice_engine::process(plugin_block& block)
{
  if(_out)
  {
    auto& mixer = get_audio_matrix_mixer(block, false);
    auto const& audio_in = mixer.mix(block, module_voice_out, 0);
    for(int c = 0; c < 2; c++) 
      audio_in[c].copy_to(block.start_frame, block.end_frame, block.voice->result[c]);
  } else 
  {
    for(int c = 0; c < 2; c++)
      block.out->voice_mixdown[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][c]);
  }
}

}
