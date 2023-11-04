#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace infernal_synth {

class audio_matrix_engine;
typedef plugin_base::jarray<
  plugin_base::jarray<float, 1> const*, 4> 
cv_matrix_mixdown;

struct matrix_module_mapping 
{ 
  int topo; 
  int slot; 
};

struct matrix_param_mapping 
{ 
  int module_topo; 
  int module_slot; 
  int param_topo; 
  int param_slot; 
};

struct param_matrix
{
  std::vector<plugin_base::list_item> items;
  std::shared_ptr<plugin_base::gui_submenu> submenu;
  std::vector<matrix_param_mapping> mappings;
};

struct module_matrix
{
  std::vector<plugin_base::list_item> items;
  std::shared_ptr<plugin_base::gui_submenu> submenu;
  std::vector<matrix_module_mapping> mappings;
};

class audio_matrix_mixer
{
  audio_matrix_engine* _engine;
public:
  INF_PREVENT_ACCIDENTAL_COPY(audio_matrix_mixer);
  audio_matrix_mixer(audio_matrix_engine* engine) : _engine(engine) {}
  plugin_base::jarray<float, 2> const& mix(plugin_base::plugin_block& block, int module, int slot);
}; 

enum {
  module_glfo, module_vlfo, module_env, module_cv_matrix, 
  module_audio_matrix, module_osc, module_fx, module_delay,
  module_monitor, module_count };

inline cv_matrix_mixdown const&
get_cv_matrix_mixdown(plugin_base::plugin_block& block)
{
  void* context = block.voice->all_context[module_cv_matrix][0];
  return *static_cast<cv_matrix_mixdown const*>(context);
}

param_matrix
make_param_matrix(std::vector<plugin_base::module_topo const*> const& modules);
module_matrix
make_module_matrix(std::vector<plugin_base::module_topo const*> const& modules);

std::unique_ptr<plugin_base::plugin_topo> synth_topo();
plugin_base::module_topo env_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo osc_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo delay_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos);
plugin_base::module_topo lfo_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, bool global);
plugin_base::module_topo fx_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, int osc_slot_count);
plugin_base::module_topo monitor_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos, int polyphony);
plugin_base::module_topo cv_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos,
  std::vector<plugin_base::module_topo const*> const& sources, std::vector<plugin_base::module_topo const*> const& targets);
plugin_base::module_topo audio_matrix_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos,
  std::vector<plugin_base::module_topo const*> const& sources, std::vector<plugin_base::module_topo const*> const& targets);

}
