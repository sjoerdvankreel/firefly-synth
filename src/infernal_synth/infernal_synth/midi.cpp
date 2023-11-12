#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { output_cp, output_pb, output_cc };

class midi_engine :
public module_engine {
public:
  void initialize() override { }
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_engine);
};

module_topo
midi_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{4E70A342-A95A-4860-8223-D4F029E22874}", "MIDI", module_midi, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info("{D38E46EA-4064-410C-BB33-DB6DA418463B}", "CP", output_cp, 1)),
      make_module_dsp_output(true, make_topo_info("{C3A35C9F-3F80-4DE0-8C8D-D18D340F9DBC}", "PB", output_pb, 1)),
      make_module_dsp_output(true, make_topo_info("{023C1F1C-873C-4D43-9469-6F36D948EE7A}", "CC", output_cc, 128, false)) }),
    make_module_gui_none(section)));

  result.midi_sources = midi_source::all_sources();
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<midi_engine>(); };

  return result;
}

void
midi_engine::process(plugin_block& block)
{
  auto& own_cv = block.state.own_cv;    
  auto const& midi = block.state.own_midi_automation;
  midi[midi_source_cp].copy_to(block.start_frame, block.end_frame, own_cv[output_cp][0]);
  midi[midi_source_pb].copy_to(block.start_frame, block.end_frame, own_cv[output_pb][0]);
  for(int i = 0; i < 128; i++)
    midi[midi_source_cc + i].copy_to(block.start_frame, block.end_frame, own_cv[output_cc][i]);
}

}
