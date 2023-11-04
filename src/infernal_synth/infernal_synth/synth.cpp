#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/support.hpp>

using namespace juce;
using namespace plugin_base;

namespace infernal_synth {

static gui_colors
make_module_colors(Colour const& c)
{
  gui_colors result;
  result.tab_text = c;
  result.knob_thumb = c;
  result.knob_track2 = c;
  result.slider_thumb = c;
  result.control_tick = c;
  result.bubble_outline = c;
  result.section_outline1 = c.darker(1);
  return result;
}

module_matrix
make_module_matrix(std::vector<plugin_base::module_topo const*> const& modules)
{
  int index = 0;
  module_matrix result;
  result.submenu = std::make_shared<gui_submenu>();
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& tag = modules[m]->info.tag;
    auto module_submenu = std::make_shared<gui_submenu>();
    module_submenu->name = tag.name;
    for (int mi = 0; mi < modules[m]->info.slot_count; mi++)
    {
      list_item item;
      item.id = tag.id + "-" + std::to_string(mi);
      item.name = tag.name + " " + std::to_string(mi + 1);
      result.items.push_back(item);
      module_submenu->indices.push_back(index++);
      result.mappings.push_back({ modules[m]->info.index, mi });
    }
    result.submenu->children.push_back(module_submenu);
  }
  return result;
}

param_matrix
make_param_matrix(std::vector<plugin_base::module_topo const*> const& modules)
{
  int index = 0;
  param_matrix result;
  result.submenu = std::make_shared<gui_submenu>();
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& module_tag = modules[m]->info.tag;
    auto module_submenu = std::make_shared<gui_submenu>();
    module_submenu->name = module_tag.name;
    for (int mi = 0; mi < modules[m]->info.slot_count; mi++)
    {
      auto module_slot_submenu = std::make_shared<gui_submenu>();
      module_slot_submenu->name = module_tag.name + " " + std::to_string(mi + 1);
      for (int p = 0; p < modules[m]->params.size(); p++)
        if (modules[m]->params[p].dsp.can_modulate(mi))
        {
          auto const& param_tag = modules[m]->params[p].info.tag;
          for (int pi = 0; pi < modules[m]->params[p].info.slot_count; pi++)
          {
            list_item item;
            item.id = module_tag.id + "-" + std::to_string(mi) + "-" + param_tag.id + "-" + std::to_string(pi);
            item.name = module_tag.name + " " + std::to_string(mi + 1) + " " + param_tag.name;
            if (modules[m]->params[p].info.slot_count > 1) item.name += " " + std::to_string(pi + 1);
            result.items.push_back(item);
            module_slot_submenu->indices.push_back(index++);
            result.mappings.push_back({ modules[m]->info.index, mi, modules[m]->params[p].info.index, pi });
          }
        }
      module_submenu->children.push_back(module_slot_submenu);
    }
    result.submenu->children.push_back(module_submenu);
  }
  return result;
}

enum { 
  section_lfos, section_env, section_osc, section_filter, section_cv_matrix, 
  section_audio_matrix, section_delay, section_monitor, section_count };

std::unique_ptr<plugin_topo>
synth_topo()
{
  gui_colors cv_colors(make_module_colors(Colour(0xFFFF8844)));
  gui_colors audio_colors(make_module_colors(Colour(0xFF4488FF)));
  gui_colors other_colors(make_module_colors(Colour(0xFFFF4488)));

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
  result->gui.aspect_ratio_width = 3;
  result->gui.aspect_ratio_height = 1;
  result->gui.dimension.row_sizes = std::vector<int>(6, 1);
  result->gui.dimension.column_sizes = std::vector<int>(2, 1);
  result->gui.typeface_file_name = "Handel Gothic Regular.ttf";

  result->gui.sections.resize(section_count);
  result->gui.sections[section_env] = make_module_section_gui(section_env, { 2, 0 }, { 1, 1 });
  result->gui.sections[section_osc] = make_module_section_gui(section_osc, { 3, 0 }, { 1, 1 });
  result->gui.sections[section_lfos] = make_module_section_gui(section_lfos, { 1, 0 }, { 1, 2 });
  result->gui.sections[section_delay] = make_module_section_gui(section_delay, { 5, 0 }, { 1, 1 });
  result->gui.sections[section_filter] = make_module_section_gui(section_filter, { 4, 0 }, { 1, 1 });
  result->gui.sections[section_monitor] = make_module_section_gui(section_monitor, { 0, 0 }, { 1, 1 });
  result->gui.sections[section_cv_matrix] = make_module_section_gui(section_cv_matrix, { 0, 1, 3, 1 }, { 1, 1 });
  result->gui.sections[section_audio_matrix] = make_module_section_gui(section_audio_matrix, { 3, 1, 3, 1 }, { 1, 1 });

  result->modules.resize(module_count);
  result->modules[module_env] = env_topo(section_env, cv_colors, { 0, 0 });
  result->modules[module_osc] = osc_topo(section_osc, audio_colors, { 0, 0 });
  result->modules[module_delay] = delay_topo(section_delay, audio_colors, { 0, 0 });
  result->modules[module_glfo] = lfo_topo(section_lfos, cv_colors, { 0, 0 }, true);
  result->modules[module_vlfo] = lfo_topo(section_lfos, cv_colors, { 0, 1 }, false);
  result->modules[module_monitor] = monitor_topo(section_monitor, other_colors, { 0, 0 }, result->polyphony);
  result->modules[module_fx] = fx_topo(section_filter, audio_colors, { 0, 0 }, result->modules[module_osc].info.slot_count);
  result->modules[module_cv_matrix] = cv_matrix_topo(section_cv_matrix, cv_colors, { 0, 0 },
    { &result->modules[module_env], &result->modules[module_vlfo], &result->modules[module_glfo] },
    { &result->modules[module_osc], &result->modules[module_fx] });
  result->modules[module_audio_matrix] = audio_matrix_topo(section_audio_matrix, audio_colors, { 0, 0 },
    { &result->modules[module_osc], &result->modules[module_fx] },
    { &result->modules[module_delay] });
  return result;
}

}