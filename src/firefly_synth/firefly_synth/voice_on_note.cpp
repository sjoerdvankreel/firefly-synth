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

enum { output_on_note_1, output_on_note_2, output_count };

class voice_on_note_engine :
public module_engine {
public:
  void initialize() override { }
  void process(plugin_block& block) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_on_note_engine);
};

module_topo
voice_on_note_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{EF1A4E73-BCAD-4D38-A54E-44B83EF46CB5}", "On Note", module_voice_on_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info("{63A2F991-5403-45BF-BC42-CF03CA56BF26}", "On Note 1", output_on_note_1, 1)),
      make_module_dsp_output(true, make_topo_info("{1D909BEE-0447-47D8-8FA8-7A3DF5605AFF}", "On Note 2", output_on_note_2, 1)) }),
      make_module_gui_none(section)));
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<voice_on_note_engine>(); };
  return result;
}

void
voice_on_note_engine::process(plugin_block& block)
{  
}

}
