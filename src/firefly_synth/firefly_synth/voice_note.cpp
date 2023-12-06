#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { output_key, output_velo, output_count };

class voice_note_engine :
public module_engine {
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const*) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_note_engine);
};

module_topo
voice_note_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{4380584E-6CC5-4DA5-A533-17A9A1777476}", "Note", module_voice_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info("{376846A2-33FC-4DB0-BCB9-7A43A8488A7F}", "Key", output_key, 1)),
      make_module_dsp_output(true, make_topo_info("{2D59B6B8-3B08-430C-9A8A-E882C8E14597}", "Velo", output_velo, 1)) }),
      make_module_gui_none(section)));
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<voice_note_engine>(); };
  return result;
}

void
voice_note_engine::process(plugin_block& block)
{  
  block.state.own_cv[output_velo][0].fill(block.start_frame, block.end_frame, block.voice->state.velocity);
  block.state.own_cv[output_key][0].fill(block.start_frame, block.end_frame, std::clamp(block.voice->state.id.key / 127.0f, 0.0f, 1.0f));
}

}
