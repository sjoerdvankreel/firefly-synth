#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/support.hpp>

using namespace plugin_base;

namespace infernal_synth {

enum { 
  section_glfo, section_vlfo, section_env, section_osc, 
  section_filter, section_cv_matrix, section_delay, section_monitor, section_count  };

std::unique_ptr<plugin_topo>
synth_topo()
{
  auto result = std::make_unique<plugin_topo>();
  result->polyphony = 32;
  result->extension = "infpreset";
  result->type = plugin_type::synth;
  result->tag.id = INF_SYNTH_ID;
  result->tag.name = INF_SYNTH_NAME;
  result->version_minor = INF_SYNTH_VERSION_MINOR;
  result->version_major = INF_SYNTH_VERSION_MAJOR;

  result->gui.min_width = 1000;
  result->gui.max_width = 2000;
  result->gui.default_width = 1000;
  result->gui.aspect_ratio_width = 12;
  result->gui.aspect_ratio_height = 5;
  result->gui.dimension.row_sizes = std::vector<int>(8, 1);
  result->gui.dimension.column_sizes = std::vector<int>(8, 1);

  result->gui.sections.resize(section_count);
  result->gui.sections[section_env] = make_module_section_gui({ 2, 0, 2, 4 } , { 1, 1 });
  result->gui.sections[section_osc] = make_module_section_gui({ 4, 0, 2, 4 }, { 1, 1 });
  result->gui.sections[section_delay] = make_module_section_gui({ 6, 4, 1, 4 }, { 1, 1 });
  result->gui.sections[section_glfo] = make_module_section_gui({ 0, 0, 2, 2 }, { 1, 1 });
  result->gui.sections[section_vlfo] = make_module_section_gui({ 0, 0, 2, 2 }, { 1, 1 });
  result->gui.sections[section_monitor] = make_module_section_gui({ 7, 4, 1, 4 }, { 1, 1 });
  result->gui.sections[section_filter] = make_module_section_gui({ 7, 4, 1, 4 }, { 1, 1 });
  result->gui.sections[section_cv_matrix] = make_module_section_gui({ 0, 4, 6, 4 }, { 1, 1 });

  result->modules.resize(module_count);
  result->modules[module_env] = env_topo(section_env, { 1, 1 });
  result->modules[module_osc] = osc_topo(section_osc, { 1, 1 });
  result->modules[module_delay] = delay_topo(section_delay, { 1, 1 });
  result->modules[module_glfo] = lfo_topo(section_glfo, { 1, 1 }, true);
  result->modules[module_vlfo] = lfo_topo(section_vlfo, { 1, 1 }, false);
  result->modules[module_monitor] = monitor_topo(section_monitor, { 1, 1 }, result->polyphony);
  result->modules[module_filter] = filter_topo(section_filter, { 1, 1 }, result->modules[module_osc].info.slot_count);
  result->modules[module_cv_matrix] = cv_matrix_topo(section_cv_matrix, { 1, 1 },
    { &result->modules[module_glfo], &result->modules[module_vlfo], &result->modules[module_env] },
    { &result->modules[module_osc], &result->modules[module_filter] });
  return result;
}

}