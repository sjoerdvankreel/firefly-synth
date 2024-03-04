#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>

#include <firefly_synth/synth.hpp>

using namespace plugin_base;

namespace firefly_synth {

enum { output_ext_audio, output_count };

class external_audio_engine :
public module_engine {
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const*) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(external_audio_engine);
};

module_topo
external_audio_topo(int section, bool is_fx)
{
  module_topo result(make_module(
    make_topo_info("{B5D634E6-4D8A-4C49-9926-1CE21C9B465F}", true, "External Audio", "External Audio", "Ext", module_external_audio, 1),
    make_module_dsp(module_stage::input, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{EA060413-A2E3-4AD2-98D0-F6FE5F7B987E}", true, "External Audio", "External Audio", "Ext", output_ext_audio, 1)) }),
    make_module_gui_none(section)));
  result.info.description = "In FX mode, provides external audio input to the global audio matrix.";
  result.engine_factory = nullptr;
  if(is_fx) result.engine_factory = [](auto const&, int, int) { return std::make_unique<external_audio_engine>(); };
  return result;
}

void
external_audio_engine::process(plugin_block& block)
{  
  float const* const* host_audio = block.host.audio_in;
  auto& own_audio = block.state.own_audio[output_ext_audio][0];
  for(int c = 0; c < 2; c++)
    std::copy(host_audio[c] + block.start_frame, host_audio[c] + block.end_frame, own_audio[c].begin() + block.start_frame);
}

}
