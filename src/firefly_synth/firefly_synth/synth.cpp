#include <firefly_synth/synth.hpp>
#include <firefly_synth/plugin.hpp>

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/gui/graph.hpp>
#include <plugin_base/gui/controls.hpp>
#include <plugin_base/gui/containers.hpp>

using namespace juce;
using namespace plugin_base;

namespace firefly_synth {

enum {
  custom_section_title,
  custom_section_hover_graph,
  custom_section_tweak_graph,
  custom_section_controls,
  custom_section_count };

enum { 
  module_section_hidden, module_section_master_in, module_section_voice_in, 
  module_section_lfos, module_section_env, module_section_cv_matrix,
  module_section_audio_matrix, module_section_osc, module_section_fxs,
  module_section_monitor_out, module_section_count };

static gui_colors
make_section_colors(Colour const& c)
{
  gui_colors result;
  result.tab_text = c;
  result.knob_thumb = c;
  result.knob_track2 = c;
  result.slider_thumb = c;
  result.control_tick = c;
  result.bubble_outline = c;
  result.scrollbar_thumb = c;
  result.graph_foreground = c;
  result.table_header = c.darker(0.3f);
  result.section_outline1 = c.darker(1);
  return result;
}

static Component&
make_graph_section(plugin_gui* gui, lnf* lnf, component_store store, bool render_on_hover)
{
  module_graph_params params;
  params.fps = 10;
  params.render_on_hover = render_on_hover;
  params.render_on_tweak = !render_on_hover;
  return store_component<module_graph>(store, gui, lnf, params);
}

static Component&
make_controls_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  auto& result = store_component<grid_component>(store, gui_dimension { 2, 4 }, 2);
  result.add(gui->make_load_button(), {0, 0});
  result.add(gui->make_save_button(), {0, 1});
  result.add(gui->make_init_button(), {0, 2});
  result.add(gui->make_clear_button(), {0, 3});
  result.add(store_component<preset_button>(store, gui), { 1, 3 });
  result.add(store_component<last_tweaked_label>(store, gui->gui_state(), "Tweak:"), {1, 0, 1, 2});
  result.add(store_component<last_tweaked_editor>(store, gui->gui_state(), lnf), { 1, 2 });
  return result;
}

static Component&
make_title_section(plugin_gui* gui, lnf* lnf, component_store store, Colour const& color)
{
  auto& grid = store_component<grid_component>(store, gui_dimension({ { 1 }, { gui_dimension::auto_size, 1 } }), 2, 0, 1);
  grid.add(store_component<image_component>(store, gui->gui_state()->desc().config, "firefly.png", RectanglePlacement::xLeft), { 0, 1 });
  auto& label = store_component<autofit_label>(store, lnf, "FIREFLY SYNTH", true, 14);
  label.setColour(Label::ColourIds::textColourId, color);
  grid.add(label, { 0, 0 });
  return grid;
}

std::unique_ptr<tab_menu_handler>
make_audio_routing_menu_handler(plugin_state* state, bool global)
{
  auto cv_params = make_audio_routing_cv_params(state, global);
  auto audio_params = make_audio_routing_audio_params(state, global);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, audio_params);
}

cv_matrix_mixdown
make_static_cv_matrix_mixdown(plugin_block& block)
{
  cv_matrix_mixdown result;
  plugin_dims dims(block.plugin);
  result.resize(dims.module_slot_param_slot);
  for (int m = 0; m < block.plugin.modules.size(); m++)
  {
    auto const& module = block.plugin.modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        if (param.dsp.can_modulate(mi))
          for (int pi = 0; pi < param.info.slot_count; pi++)
            result[m][mi][p][pi] = &block.state.all_accurate_automation[m][mi][p][pi];
      }
  }
  return result;
}

std::vector<module_topo const*>
make_audio_matrix_sources(plugin_topo const* topo, bool global)
{
  if (global)
    return { &topo->modules[module_voice_mix], &topo->modules[module_gfx] };
  else
    return { &topo->modules[module_osc], &topo->modules[module_vfx] };
}

std::vector<module_topo const*>
make_audio_matrix_targets(plugin_topo const* topo, bool global)
{
  if (global)
    return { &topo->modules[module_gfx], &topo->modules[module_master_out] };
  else
    return { &topo->modules[module_vfx], &topo->modules[module_voice_out] };
}

std::vector<module_topo const*>
make_cv_matrix_targets(plugin_topo const* topo, bool global)
{
  if (global)
    return {
      &topo->modules[module_gfx], 
      &topo->modules[module_gaudio_matrix], 
      &topo->modules[module_master_out] };
  else
    return { 
      &topo->modules[module_voice_in],
      &topo->modules[module_osc], 
      &topo->modules[module_vfx], 
      &topo->modules[module_vaudio_matrix], 
      &topo->modules[module_voice_out] };
}

std::vector<cv_source_entry>
make_cv_matrix_sources(plugin_topo const* topo, bool global)
{
  if(global)
    return { 
      { "", &topo->modules[module_master_in]},
      { "", &topo->modules[module_glfo] },
      { "", &topo->modules[module_midi] } };
  else
    return { 
      { "Global", nullptr }, 
      { "", &topo->modules[module_master_in]},
      { "", &topo->modules[module_glfo] },
      { "", &topo->modules[module_midi] },
      { "Voice", nullptr }, 
      { "", &topo->modules[module_env] },
      { "", &topo->modules[module_vlfo] },
      { "", &topo->modules[module_voice_note] },
      { "", &topo->modules[module_voice_on_note] }};
}

std::unique_ptr<plugin_topo>
synth_topo()
{
  Colour custom_color(0xFFFF4488);
  Colour global_color(0xFF4488FF);
  gui_colors custom_colors(make_section_colors(custom_color));
  gui_colors monitor_colors(make_section_colors(global_color));
  gui_colors voice_colors(make_section_colors(Colour(0xFFFF8844)));
  gui_colors matrix_colors(make_section_colors(Colour(0xFF8888FF)));
  gui_colors global_colors(make_section_colors(Colour(global_color)));
  custom_colors.edit_text = custom_color;
  monitor_colors.control_text = global_color;

  auto result = std::make_unique<plugin_topo>();
  result->polyphony = 32;
  result->extension = "ffpreset";
  result->vendor = "Sjoerd van Kreel";
  result->type = plugin_type::synth;
  result->tag.id = FF_SYNTH_ID;
  result->tag.name = FF_SYNTH_NAME;
  result->version_minor = FF_SYNTH_VERSION_MINOR;
  result->version_major = FF_SYNTH_VERSION_MAJOR;

  result->gui.min_width = 820;
  result->gui.aspect_ratio_width = 121;
  result->gui.aspect_ratio_height = 56;
  result->gui.dimension.column_sizes = { 16, 12, 12, 13, 13 };
  result->gui.dimension.row_sizes = { 1, 1, 1, 1, 1, 1, 1, 1 };
  result->gui.typeface_file_name = "Handel Gothic Regular.ttf";

  result->gui.custom_sections.resize(custom_section_count);
  auto make_title_section_ui = [custom_color](plugin_gui* gui, lnf* lnf, auto store) -> Component& { 
    return make_title_section(gui, lnf, store, custom_color); };
  result->gui.custom_sections[custom_section_title] = make_custom_section_gui(
    custom_section_title, { 0, 0, 1, 1 }, custom_colors, make_title_section_ui);
  result->gui.custom_sections[custom_section_controls] = make_custom_section_gui(
    custom_section_controls, { 0, 3, 1, 2 }, custom_colors, make_controls_section);
  result->gui.custom_sections[custom_section_hover_graph] = make_custom_section_gui(
    custom_section_hover_graph, { 0, 1, 1, 1 }, custom_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_graph_section(gui, lnf, store, true); });
  result->gui.custom_sections[custom_section_tweak_graph] = make_custom_section_gui(
    custom_section_tweak_graph, { 0, 2, 1, 1 }, custom_colors, [](auto* gui, auto* lnf, auto store)
      -> Component& { return make_graph_section(gui, lnf, store, false); });

  result->gui.module_sections.resize(module_section_count);
  result->gui.module_sections[module_section_hidden] = make_module_section_gui_none(
    "{F289D07F-0A00-4AB1-B87B-685CB4D8B2F8}", module_section_hidden);
  result->gui.module_sections[module_section_lfos] = make_module_section_gui(
    "{0DA0E7C3-8DBB-440E-8830-3B6087F23B81}", module_section_lfos, { 2, 0, 1, 3 }, { 1, 2 });
  result->gui.module_sections[module_section_env] = make_module_section_gui(
    "{AB26F56E-DC6D-4F0B-845D-C750728F8FA2}", module_section_env, { 3, 0, 1, 3 }, { 1, 1 });
  result->gui.module_sections[module_section_osc] = make_module_section_gui(
    "{7A457CCC-E719-4C07-98B1-017EA7DEFB1F}", module_section_osc, { 5, 0, 1, 3 }, { 1, 1 });
  result->gui.module_sections[module_section_fxs] = make_module_section_gui(
    "{96C75EE5-577E-4508-A85A-E92FF9FD8A4D}", module_section_fxs, { 6, 0, 1, 3 }, { 1, 2 });
  result->gui.module_sections[module_section_master_in] = make_module_section_gui(
    "{F9578AAA-66A4-4B0C-A941-4719B5F0E998}", module_section_master_in, { 1, 0, 1, 3 }, { 1, 1 });
  result->gui.module_sections[module_section_voice_in] = make_module_section_gui(
    "{FB435C64-8349-4F0F-84FC-FFC82002D69F}", module_section_voice_in, { 4, 0, 1, 3 }, { 1, 1 });
  result->gui.module_sections[module_section_monitor_out] = make_module_section_gui(
    "{8FDAEB21-8876-4A90-A8E1-95A96FB98FD8}", module_section_monitor_out, { 7, 0, 1, 3 }, { { 1 }, { 1, 1, 2 } });
  result->gui.module_sections[module_section_cv_matrix] = make_module_section_gui_tabbed(
    "{11A46FE6-9009-4C17-B177-467243E171C8}", module_section_cv_matrix, { 1, 3, 4, 2 },
    "CV", result->gui.module_header_width, { module_vcv_matrix, module_gcv_matrix });
  result->gui.module_sections[module_section_audio_matrix] = make_module_section_gui_tabbed(
    "{950B6610-5CE1-4629-943F-CB2057CA7346}", module_section_audio_matrix, { 5, 3, 3, 2 },
    "Audio", result->gui.module_header_width, { module_vaudio_matrix, module_gaudio_matrix });

  result->modules.resize(module_count);
  result->modules[module_midi] = midi_topo(module_section_hidden);
  result->modules[module_voice_mix] = voice_mix_topo(module_section_hidden);
  result->modules[module_voice_note] = voice_note_topo(module_section_hidden);
  result->modules[module_env] = env_topo(module_section_env, voice_colors, { 0, 0 });
  result->modules[module_osc] = osc_topo(module_section_osc, voice_colors, { 0, 0 });
  result->modules[module_gfx] = fx_topo(module_section_fxs, global_colors, { 0, 1 }, true);
  result->modules[module_vfx] = fx_topo(module_section_fxs, voice_colors, { 0, 0 }, false);
  result->modules[module_glfo] = lfo_topo(module_section_lfos, global_colors, { 0, 1 }, true);
  result->modules[module_vlfo] = lfo_topo(module_section_lfos, voice_colors, { 0, 0 }, false);
  result->modules[module_master_in] = master_in_topo(module_section_master_in, global_colors, { 0, 0 });
  result->modules[module_voice_on_note] = voice_on_note_topo(result.get(), module_section_hidden); // must be after all global cv  
  result->modules[module_voice_in] = voice_in_topo(module_section_voice_in, voice_colors, { 0, 0 }); // must be after all cv
  result->modules[module_voice_out] = audio_out_topo(module_section_monitor_out, voice_colors, { 0, 0 }, false);
  result->modules[module_master_out] = audio_out_topo(module_section_monitor_out, global_colors, { 0, 1 }, true);
  result->modules[module_monitor] = monitor_topo(module_section_monitor_out, monitor_colors, { 0, 2 }, result->polyphony);
  result->modules[module_gaudio_matrix] = audio_matrix_topo(module_section_audio_matrix, matrix_colors, { 0, 0 }, true,
    make_audio_matrix_sources(result.get(), true), make_audio_matrix_targets(result.get(), true));
  result->modules[module_vaudio_matrix] = audio_matrix_topo(module_section_audio_matrix, matrix_colors, { 0, 0 }, false,
    make_audio_matrix_sources(result.get(), false), make_audio_matrix_targets(result.get(), false));
  result->modules[module_gcv_matrix] = cv_matrix_topo(module_section_cv_matrix, matrix_colors, { 0, 0 }, true,
    make_cv_matrix_sources(result.get(), true), {}, make_cv_matrix_targets(result.get(), true));
  result->modules[module_vcv_matrix] = cv_matrix_topo(module_section_cv_matrix, matrix_colors, { 0, 0 }, false,
    make_cv_matrix_sources(result.get(), false),
    make_cv_matrix_sources(result.get(), true),
    make_cv_matrix_targets(result.get(), false));
  return result;
}

}