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
  result->extension = "infpreset";
  result->type = plugin_type::synth;
  result->version_minor = INF_SYNTH_VERSION_MINOR;
  result->version_major = INF_SYNTH_VERSION_MAJOR;

  result->gui.min_width = 400;
  result->gui.max_width = 1200;
  result->gui.default_width = 800;
  result->gui.aspect_ratio_width = 4;
  result->gui.aspect_ratio_height = 3;
  result->gui.dimension.column_sizes = { 1 };
  result->gui.dimension.row_sizes = { 1, 1, 2, 1, 3, 1 };

  result->modules.resize(module_count);
  result->modules[module_lfo] = lfo_topo();
  result->modules[module_env] = env_topo();
  result->modules[module_osc] = osc_topo();
  result->modules[module_filter] = filter_topo(
    result->modules[module_osc].info.slot_count);
  result->modules[module_cv_matrix] = cv_matrix_topo(
    result->modules[module_lfo], result->modules[module_env], 
    result->modules[module_osc], result->modules[module_filter]);
  result->modules[module_delay] = delay_topo(result->polyphony);
  return result;
}

}