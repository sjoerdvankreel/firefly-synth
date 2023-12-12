#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace firefly_synth {

// shared by midi and cv matrix
enum { midi_output_cp, midi_output_pb, midi_output_cc };

// this describes our semi-modular synth
std::unique_ptr<plugin_base::plugin_topo> synth_topo();
enum {
  module_midi, module_master_in, module_glfo, module_gcv_matrix, module_voice_note, 
  module_voice_on_note, module_vlfo, module_env, module_vcv_matrix, module_voice_in, 
  module_vaudio_matrix, module_am_matrix, module_osc, module_vfx, module_voice_out,
  module_voice_mix, module_gaudio_matrix, module_gfx, module_master_out, module_monitor, module_count };

// gets the modulation signal to be applied by the oscillator at the end of it's process call
// (e.g. osc 2 is modulated by both osc 1 and osc 2 itself)
class am_matrix_engine;
class am_matrix_modulator
{
  am_matrix_engine* _engine;
public:
  PB_PREVENT_ACCIDENTAL_COPY(am_matrix_modulator);
  am_matrix_modulator(am_matrix_engine* engine) : _engine(engine) {}
  plugin_base::jarray<float, 2> const& modulate(plugin_base::plugin_block& block, int slot);
};

inline am_matrix_modulator&
get_am_matrix_modulator(plugin_base::plugin_block& block)
{
  void* context = block.module_context(module_am_matrix, 0);
  assert(context != nullptr);
  return *static_cast<am_matrix_modulator*>(context);
}

// gets the audio mixdown to be used as input at the beginning of an audio module 
// (e.g. combined audio input for "voice fx 3")
class audio_matrix_engine;
class audio_matrix_mixer
{
  audio_matrix_engine* _engine;
public:
  PB_PREVENT_ACCIDENTAL_COPY(audio_matrix_mixer);
  audio_matrix_mixer(audio_matrix_engine* engine) : _engine(engine) {}
  plugin_base::jarray<float, 2> const& mix(plugin_base::plugin_block& block, int module, int slot);
};

inline audio_matrix_mixer&
get_audio_matrix_mixer(plugin_base::plugin_block& block, bool global)
{
  int module = global ? module_gaudio_matrix : module_vaudio_matrix;
  void* context = block.module_context(module, 0);
  assert(context != nullptr);
  return *static_cast<audio_matrix_mixer*>(context);
}

// gets the cv mixdown for all modulatable parameters in the currently processing module
typedef plugin_base::jarray<plugin_base::jarray<float, 1> const*, 4> cv_matrix_mixdown;
inline cv_matrix_mixdown const&
get_cv_matrix_mixdown(plugin_base::plugin_block const& block, bool global)
{
  int module = global ? module_gcv_matrix : module_vcv_matrix;
  void* context = block.module_context(module, 0);
  return *static_cast<cv_matrix_mixdown const*>(context);
}

// set all outputs to current automation values
cv_matrix_mixdown
make_static_cv_matrix_mixdown(plugin_base::plugin_block& block);

// routing matrices sources/targets
std::vector<plugin_base::module_topo const*>
make_cv_matrix_targets(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::cv_source_entry>
make_cv_matrix_sources(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::module_topo const*>
make_audio_matrix_sources(plugin_base::plugin_topo const* topo, bool global);
std::vector<plugin_base::module_topo const*>
make_audio_matrix_targets(plugin_base::plugin_topo const* topo, bool global);

// menu handlers to update routing on clear/move/swap/copy
std::unique_ptr<plugin_base::tab_menu_handler>
make_cv_routing_menu_handler(plugin_base::plugin_state* state);
std::unique_ptr<plugin_base::tab_menu_handler>
make_audio_routing_menu_handler(plugin_base::plugin_state* state, bool global);
plugin_base::audio_routing_audio_params
make_audio_routing_am_params(plugin_base::plugin_state* state);
plugin_base::audio_routing_cv_params
make_audio_routing_cv_params(plugin_base::plugin_state* state, bool global);
plugin_base::audio_routing_audio_params
make_audio_routing_audio_params(plugin_base::plugin_state* state, bool global);

// these describe individual modules
plugin_base::module_topo midi_topo(int section);
plugin_base::module_topo voice_mix_topo(int section);
plugin_base::module_topo voice_note_topo(int section);
plugin_base::module_topo voice_on_note_topo(plugin_base::plugin_topo const* topo, int section);
plugin_base::module_topo env_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo osc_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo voice_in_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo master_in_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo fx_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo lfo_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo audio_out_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo monitor_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, int polyphony);
plugin_base::module_topo am_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, plugin_base::plugin_topo const* plugin);
plugin_base::module_topo audio_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global,
  std::vector<plugin_base::module_topo const*> const& sources, std::vector<plugin_base::module_topo const*> const& targets);
plugin_base::module_topo cv_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global,
  std::vector<plugin_base::cv_source_entry> const& sources, std::vector<plugin_base::cv_source_entry> const& on_note_sources, std::vector<plugin_base::module_topo const*> const& targets);

}
