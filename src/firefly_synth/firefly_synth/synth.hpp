#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace firefly_synth {

class audio_matrix_engine;
typedef plugin_base::jarray<
  plugin_base::jarray<float, 1> const*, 4> 
cv_matrix_mixdown;

extern int const master_cv_param_pb_range;
enum { midi_output_cp, midi_output_pb, midi_output_cc };
enum {
  module_midi, module_master_cv, module_glfo, module_gcv_matrix, module_voice_note, module_voice_on_note, module_voice_cv,
  module_vlfo, module_env, module_vcv_matrix, module_vaudio_matrix, module_osc, module_vfx, module_voice_audio_out,
  module_voice_audio_in, module_gaudio_matrix, module_gfx, module_master_audio_out, module_monitor, module_count };

class audio_matrix_mixer
{
  audio_matrix_engine* _engine;
public:
  PB_PREVENT_ACCIDENTAL_COPY(audio_matrix_mixer);
  audio_matrix_mixer(audio_matrix_engine* engine) : _engine(engine) {}
  plugin_base::jarray<float, 2> const&
    mix(plugin_base::plugin_block& block, int module, int slot);
};

// set all outputs to current automation values
cv_matrix_mixdown
make_static_cv_matrix_mixdown(plugin_base::plugin_block& block);

// routing matrices sources/targets
std::vector<plugin_base::module_topo const*>
make_audio_matrix_sources(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::module_topo const*>
make_audio_matrix_targets(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::module_topo const*>
make_cv_matrix_targets(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::cv_source_entry>
make_cv_matrix_sources(plugin_base::plugin_topo const* topo, bool global, std::set<int> short_name_modules);

// menu handlers to update routing on clear/move/swap/copy
std::unique_ptr<plugin_base::tab_menu_handler>
make_cv_routing_menu_handler(plugin_base::plugin_state* state);
std::unique_ptr<plugin_base::tab_menu_handler>
make_audio_routing_menu_handler(plugin_base::plugin_state* state, bool global);
plugin_base::audio_routing_cv_params
make_audio_routing_cv_params(plugin_base::plugin_state* state, bool global);
plugin_base::audio_routing_audio_params
make_audio_routing_audio_params(plugin_base::plugin_state* state, bool global);

inline audio_matrix_mixer&
get_audio_matrix_mixer(plugin_base::plugin_block& block, bool global)
{
  int module = global? module_gaudio_matrix: module_vaudio_matrix;
  void* context = block.module_context(module, 0);
  return *static_cast<audio_matrix_mixer*>(context);
}

inline cv_matrix_mixdown const&
get_cv_matrix_mixdown(plugin_base::plugin_block const& block, bool global)
{
  int module = global ? module_gcv_matrix : module_vcv_matrix;
  void* context = block.module_context(module, 0);
  return *static_cast<cv_matrix_mixdown const*>(context);
}

std::unique_ptr<plugin_base::plugin_topo> synth_topo();
plugin_base::module_topo midi_topo(int section);
plugin_base::module_topo voice_note_topo(int section);
plugin_base::module_topo voice_audio_in_topo(int section);
plugin_base::module_topo voice_on_note_topo(plugin_base::plugin_topo const* topo, int section);
plugin_base::module_topo env_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo osc_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo voice_cv_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo master_cv_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo fx_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo lfo_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo audio_out_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo monitor_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, int polyphony);
plugin_base::module_topo audio_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global,
  std::vector<plugin_base::module_topo const*> const& sources, std::vector<plugin_base::module_topo const*> const& targets);
plugin_base::module_topo cv_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global,
  std::vector<plugin_base::cv_source_entry> const& sources, std::vector<plugin_base::cv_source_entry> const& on_note_sources, std::vector<plugin_base::module_topo const*> const& targets);

}
