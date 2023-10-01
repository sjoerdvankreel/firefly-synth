#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>

#include <plugin_base/engine.hpp>

using namespace plugin_base;

namespace infernal_synth {

std::unique_ptr<plugin_topo>
synth_topo()
{
  auto result = std::make_unique<plugin_topo>();
  result->polyphony = 32;
  result->id = INF_SYNTH_ID;
  result->name = INF_SYNTH_NAME;
  result->type = plugin_type::synth;
  result->gui_min_width = 400;
  result->gui_max_width = 1200;
  result->gui_default_width = 800;
  result->gui_aspect_ratio_width = 4;
  result->gui_aspect_ratio_height = 3;
  result->preset_extension = "infpreset";
  result->dimension.column_sizes = { 1 };
  result->dimension.row_sizes = { 1, 1, 2, 1, 1 };
  result->modules.emplace_back(lfo_topo());
  result->modules.emplace_back(env_topo());
  result->modules.emplace_back(osc_topo());
  result->modules.emplace_back(filter_topo(result->modules[module_osc].slot_count));
  result->modules.emplace_back(delay_topo(result->polyphony));
  result->version_minor = INF_SYNTH_VERSION_MINOR;
  result->version_major = INF_SYNTH_VERSION_MAJOR;
  return result;
}

}