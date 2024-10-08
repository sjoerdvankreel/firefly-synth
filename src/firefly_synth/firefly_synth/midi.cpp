#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { section_main };
enum { custom_out_tag_cp, custom_out_tag_pb, custom_out_tag_cc };

class midi_engine :
public module_engine {
  float _graph_cp = {};
  float _graph_pb = {};
  std::array<float, 128> _graph_cc = {};
public:
  void reset_audio(plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override {}
  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  void reset_graph(plugin_block const* block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes, 
    std::vector<mod_out_custom_state> const& custom_outputs, 
    void* context) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_engine);
};

module_topo
midi_topo(int section)
{
  module_topo result(make_module(
    make_topo_info("{4E70A342-A95A-4860-8223-D4F029E22874}", true, "MIDI", "MIDI", "MIDI", module_midi, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_module_dsp_output(true, -1, make_topo_info_basic("{D38E46EA-4064-410C-BB33-DB6DA418463B}", "CP", midi_output_cp, 1)),
      make_module_dsp_output(true, -1, make_topo_info_basic("{C3A35C9F-3F80-4DE0-8C8D-D18D340F9DBC}", "PB", midi_output_pb, 1)),
      make_module_dsp_output(true, -1, make_topo_info("{023C1F1C-873C-4D43-9469-6F36D948EE7A}", false, "CC", "CC", "CC", midi_output_cc, 128))}),
    make_module_gui_none(section)));
  result.info.description = "Provides MIDI pitchbend, channel pressure and 128 CC parameters as modulation sources.";

  result.midi_sources = midi_source::all_sources();
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<midi_engine>(); };
  return result;
}

void 
midi_engine::reset_graph(plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes,
  std::vector<mod_out_custom_state> const& custom_outputs,
  void* context)
{
  _graph_cp = {};
  _graph_pb = {};
  _graph_cc = {};

  for(int i = 0; i < custom_outputs.size(); i++)
    if(custom_outputs[i].module_global == block->module_desc_.info.global)
      switch (custom_outputs[i].tag_custom)
      {
      case custom_out_tag_cp:
        _graph_cp = custom_outputs[i].value_custom_float();
        break;
      case custom_out_tag_pb:
        _graph_pb = custom_outputs[i].value_custom_float();
        break;
      default:
        assert(0 <= custom_outputs[i].tag_custom - 2 && custom_outputs[i].tag_custom - 2 < 128);
        _graph_cc[custom_outputs[i].tag_custom - 2] = custom_outputs[i].value_custom_float();
        break;
      }
}

void
midi_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  auto& own_cv = block.state.own_cv;    
  auto const& midi = block.state.own_midi_automation;

  // when graphing we dont care about the active midi
  // selectors, those are just a perf opt

  if (!block.graph)
  {
    if (block.state.own_midi_active_selection[midi_source_pb])
    {
      midi[midi_source_pb].copy_to(block.start_frame, block.end_frame, own_cv[midi_output_pb][0]);
      block.push_modulation_output(modulation_output::make_mod_output_custom_state_float(
        -1, block.module_desc_.info.global, custom_out_tag_pb, own_cv[midi_output_pb][0][0]));
    }
  } else 
  {
      own_cv[midi_output_pb][0].fill(block.start_frame, block.end_frame, _graph_pb);
  }

  if (!block.graph)
  {
    if (block.state.own_midi_active_selection[midi_source_cp])
    {
      midi[midi_source_cp].copy_to(block.start_frame, block.end_frame, own_cv[midi_output_cp][0]);
      block.push_modulation_output(modulation_output::make_mod_output_custom_state_float(
        -1, block.module_desc_.info.global, custom_out_tag_cp, own_cv[midi_output_cp][0][0]));
    }
  } else
  {
      own_cv[midi_output_cp][0].fill(block.start_frame, block.end_frame, _graph_cp);
  }

  for(int i = 0; i < 128; i++)
    if (!block.graph)
    {
      if (block.state.own_midi_active_selection[midi_source_cc + i])
      {
        midi[midi_source_cc + i].copy_to(block.start_frame, block.end_frame, own_cv[midi_output_cc][i]);
        block.push_modulation_output(modulation_output::make_mod_output_custom_state_float(
          -1, block.module_desc_.info.global, custom_out_tag_cc + i, own_cv[midi_output_cc][i][0]));
      }
    }
    else
    {
      own_cv[midi_output_cc][i].fill(block.start_frame, block.end_frame, _graph_cc[i]);
    }
}

}
