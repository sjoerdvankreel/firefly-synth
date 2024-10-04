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

enum { custom_tag_pitch, custom_tag_velo };
enum { output_key_pitch, output_velo, output_count };

class voice_note_engine :
public module_engine {

  float _graph_velo = 0.0f;
  float _graph_pitch = 0.0f;

public:
  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  void reset_audio(plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override {}
  void reset_graph(plugin_block const* block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes,
    std::vector<mod_out_custom_state> const& custom_outputs, 
    void* context) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_note_engine);
};

module_topo
voice_note_topo(int section)
{
  module_topo result(make_module(
    make_topo_info_basic("{4380584E-6CC5-4DA5-A533-17A9A1777476}", "Note", module_voice_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info_basic("{376846A2-33FC-4DB0-BCB9-7A43A8488A7F}", "Key", output_key_pitch, 1)),
      make_module_dsp_output(true, make_topo_info("{2D59B6B8-3B08-430C-9A8A-E882C8E14597}", true, "Velocity", "Velo", "Velo", output_velo, 1)) }),
    make_module_gui_none(section)));
  result.info.description = "Provides MIDI note and velocity as modulation sources.";
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<voice_note_engine>(); };
  return result;
}

void 
voice_note_engine::reset_graph(
  plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes,
  std::vector<mod_out_custom_state> const& custom_outputs, 
  void* context)
{
  reset_audio(block, nullptr, nullptr);

  // fix to current live values
  // backwards loop, outputs are sorted, latest-in-time are at the end
  bool seen_velo = false;
  bool seen_pitch = false;
  if (custom_outputs.size())
    for (int i = (int)custom_outputs.size() - 1; i >= 0; i--)
    {
      if (custom_outputs[i].module_global == block->module_desc_.info.global)
      {
        if (!seen_velo && custom_outputs[i].tag_custom == custom_tag_velo)
        {
          seen_velo = true;
          _graph_velo = *reinterpret_cast<float const*>(&custom_outputs[i].value_custom);
        }
        if (!seen_pitch && custom_outputs[i].tag_custom == custom_tag_pitch)
        {
          seen_pitch = true;
          _graph_pitch = *reinterpret_cast<float const*>(&custom_outputs[i].value_custom);
        }
        if (seen_velo && seen_pitch)
          break;
      }
    }
}

void
voice_note_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{  
  float velo;
  float pitch;

  if (block.graph)
  {
    velo = _graph_velo;
    pitch = _graph_pitch;
  }
  else
  {
    velo = block.voice->state.velocity;
    pitch = block.voice->state.note_id_.key / 127.0f;
    // we don't have a default 12-tet tuning table
    if (block.current_tuning_mode != engine_tuning_mode_no_tuning)
      pitch = (*block.current_tuning)[block.voice->state.note_id_.key].retuned_semis / 127.0f;
  }

  block.state.own_cv[output_velo][0].fill(block.start_frame, block.end_frame, velo);
  block.state.own_cv[output_key_pitch][0].fill(block.start_frame, block.end_frame, pitch);

  if (block.graph) return;

  block.push_modulation_output(modulation_output::make_mod_output_custom_state(
    block.voice->state.slot,
    block.module_desc_.info.global,
    custom_tag_pitch,
    *reinterpret_cast<int*>(&pitch)));
  block.push_modulation_output(modulation_output::make_mod_output_custom_state(
    block.voice->state.slot,
    block.module_desc_.info.global,
    custom_tag_velo,
    *reinterpret_cast<int*>(&velo)));
}

}
