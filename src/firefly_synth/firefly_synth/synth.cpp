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
  custom_section_main_graph,
  custom_section_controls,
  custom_section_gfx_graph,
  custom_section_glfo_graph,
  custom_section_osc_graph,
  custom_section_vfx_graph,
  custom_section_vlfo_graph,
  custom_section_env_graph,
  custom_section_matrix_graphs,
  custom_section_count };

enum { 
  module_section_hidden, 
  module_section_master_in, module_section_master_out,
  module_section_gfx, module_section_glfo,
  module_section_voice_in, module_section_voice_out,
  module_section_osc, module_section_vfx, 
  module_section_vlfo, module_section_env, 
  module_section_monitor, module_section_matrices, 
  module_section_count };

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

static module_graph_params
make_module_graph_params(int module, bool render_on_module_mouse_enter, 
  bool render_on_param_mouse_enter, std::vector<int> const& dependent_module_indices)
{
  module_graph_params result;
  result.fps = 10;
  result.module_index = module;
  result.render_on_tweak = true;
  result.render_on_tab_change = true;
  result.dependent_module_indices = dependent_module_indices;
  result.render_on_module_mouse_enter = render_on_module_mouse_enter;
  if(render_on_param_mouse_enter)
    result.render_on_param_mouse_enter_modules = { -1 };
  return result;
}

static Component&
make_module_graph_section(
  plugin_gui* gui, lnf* lnf, component_store store, int module, bool render_on_module_mouse_enter,
  bool render_on_param_mouse_enter, std::vector<int> const& dependent_module_indices)
{
  module_graph_params params = make_module_graph_params(module, 
    render_on_module_mouse_enter, render_on_param_mouse_enter, dependent_module_indices);
  return store_component<module_graph>(store, gui, lnf, params);
}

static Component&
make_main_graph_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  module_graph_params params;
  params.fps = 10;
  params.module_index = -1;
  params.render_on_tweak = true;
  params.render_on_tab_change = false;
  params.render_on_module_mouse_enter = true;
  params.render_on_param_mouse_enter_modules = { 
    module_master_in, module_voice_in, module_am_matrix, module_vaudio_matrix, 
    module_gaudio_matrix, module_vcv_matrix, module_gcv_matrix };
  return store_component<module_graph>(store, gui, lnf, params);
}

static Component&
make_matrix_graphs_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  return store_component<tabbed_module_section_container>(store, gui, module_section_matrices, 
    [gui, lnf](int module_index) -> std::unique_ptr<juce::Component> {
      switch (module_index)
      {
      case module_am_matrix: return std::make_unique<module_graph>(gui, lnf, make_module_graph_params(module_index, true, false, { module_osc } ));
      case module_vaudio_matrix: return std::make_unique<module_graph>(gui, lnf, make_module_graph_params(module_index, false, true, { }));
      case module_gaudio_matrix: return std::make_unique<module_graph>(gui, lnf, make_module_graph_params(module_index, false, true, { }));
      case module_vcv_matrix: return std::make_unique<module_graph>(gui, lnf, make_module_graph_params(module_index, false, true, 
        { module_master_in, module_glfo, module_vlfo, module_env, module_voice_in, module_voice_out,
        module_osc, module_am_matrix, module_vfx, module_vlfo, module_vaudio_matrix }));
      case module_gcv_matrix: return std::make_unique<module_graph>(gui, lnf, make_module_graph_params(module_index, false, true, 
        { module_master_in, module_glfo, module_gfx, module_gaudio_matrix, module_master_out }));
      default: assert(false); return std::make_unique<Component>();
      }
    });
}

static Component&
make_controls_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  auto& result = store_component<grid_component>(store, gui_dimension{ 2, 5 }, 2);
  result.add(gui->make_load_button(), { 0, 0 });
  result.add(gui->make_save_button(), { 0, 1 });
  result.add(gui->make_init_button(), { 0, 2 });
  result.add(gui->make_clear_button(), { 0, 3 });
  result.add(store_component<preset_button>(store, gui), { 0, 4 });
  auto& tweak_label = store_component<last_tweaked_label>(store, gui->gui_state());
  tweak_label.setJustificationType(Justification::centredRight);
  result.add(tweak_label, { 1, 0, 1, 3 });
  result.add(store_component<last_tweaked_editor>(store, gui->gui_state(), lnf), { 1, 3, 1, 2 });
  return result;
}

static Component&
make_title_section(plugin_gui* gui, lnf* lnf, component_store store, Colour const& color)
{
  auto& grid = store_component<grid_component>(store, gui_dimension({ { 1 }, { gui_dimension::auto_size, 1 } }), 2, 0, 1);
  grid.add(store_component<image_component>(store, gui->gui_state()->desc().config, "firefly.png", RectanglePlacement::xRight), { 0, 1 });
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
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params }));
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
      &topo->modules[module_am_matrix],
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
  gui_colors custom_colors(make_section_colors(custom_color));
  gui_colors monitor_colors(make_section_colors(custom_color));
  gui_colors voice_colors(make_section_colors(Colour(0xFFFF8844)));
  gui_colors global_colors(make_section_colors(Colour(0xFF4488FF)));
  gui_colors matrix_colors(make_section_colors(Colour(0xFF8888FF)));
  custom_colors.edit_text = custom_color;
  monitor_colors.control_text = custom_color;

  auto result = std::make_unique<plugin_topo>();
  result->polyphony = 32;
  result->extension = "ffpreset";
  result->vendor = "Sjoerd van Kreel";
  result->type = plugin_type::synth;
  result->tag.id = FF_SYNTH_ID;
  result->tag.name = FF_SYNTH_NAME;
  result->version_minor = FF_SYNTH_VERSION_MINOR;
  result->version_major = FF_SYNTH_VERSION_MAJOR;

  result->gui.min_width = 944;
  result->gui.aspect_ratio_width = 23;
  result->gui.aspect_ratio_height = 11;
  result->gui.dimension.column_sizes = { 8, 12, 5, 13 };
  result->gui.typeface_file_name = "Handel Gothic Regular.ttf";
  int height = result->gui.min_width * result->gui.aspect_ratio_height / result->gui.aspect_ratio_width;
  result->gui.dimension.row_sizes = gui_vertical_distribution(height, result->gui.font_height, 
    { { true, 1 }, { true, 1 }, { true, 1 }, { true, 1 }, { true, 1 }, { true, 1 }, { true, 1 }, { true, 1 }, { true, 2 } });

  result->gui.custom_sections.resize(custom_section_count);
  auto make_title_section_ui = [custom_color](plugin_gui* gui, lnf* lnf, auto store) -> Component& { 
    return make_title_section(gui, lnf, store, custom_color); };
  result->gui.custom_sections[custom_section_title] = make_custom_section_gui(
    custom_section_title, { 0, 0, 1, 1 }, custom_colors, make_title_section_ui);
  result->gui.custom_sections[custom_section_controls] = make_custom_section_gui(
    custom_section_controls, { 0, 3, 1, 1 }, custom_colors, make_controls_section);
  result->gui.custom_sections[custom_section_main_graph] = make_custom_section_gui(
    custom_section_main_graph, { 0, 2, 1, 1 }, custom_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_main_graph_section(gui, lnf, store); });
  result->gui.custom_sections[custom_section_gfx_graph] = make_custom_section_gui(
    custom_section_gfx_graph, { 2, 2, 1, 1 }, global_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_gfx, false, false, {}); });
  result->gui.custom_sections[custom_section_glfo_graph] = make_custom_section_gui(
    custom_section_glfo_graph, { 3, 2, 1, 1 }, global_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_glfo, false, false, {}); });
  result->gui.custom_sections[custom_section_osc_graph] = make_custom_section_gui(
    custom_section_osc_graph, { 5, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_osc, false, false, { module_am_matrix }); });
  result->gui.custom_sections[custom_section_vfx_graph] = make_custom_section_gui(
    custom_section_vfx_graph, { 6, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_vfx, false, false, {}); });
  result->gui.custom_sections[custom_section_vlfo_graph] = make_custom_section_gui(
    custom_section_vlfo_graph, { 7, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_vlfo, false, false, {}); });
  result->gui.custom_sections[custom_section_env_graph] = make_custom_section_gui(
    custom_section_env_graph, { 8, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_env, false, false, {}); });
  result->gui.custom_sections[custom_section_matrix_graphs] = make_custom_section_gui(
    custom_section_matrix_graphs, { 8, 3, 1, 1 }, matrix_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_matrix_graphs_section(gui, lnf, store); });

  result->gui.module_sections.resize(module_section_count);
  result->gui.module_sections[module_section_hidden] = make_module_section_gui_none(
    "{F289D07F-0A00-4AB1-B87B-685CB4D8B2F8}", module_section_hidden);
  result->gui.module_sections[module_section_vlfo] = make_module_section_gui(
    "{0DA0E7C3-8DBB-440E-8830-3B6087F23B81}", module_section_vlfo, { 7, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_vfx] = make_module_section_gui(
    "{5BD5F320-3CA9-490B-B69E-36A003F41CEC}", module_section_vfx, { 6, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_env] = make_module_section_gui(
    "{AB26F56E-DC6D-4F0B-845D-C750728F8FA2}", module_section_env, { 8, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_osc] = make_module_section_gui(
    "{7A457CCC-E719-4C07-98B1-017EA7DEFB1F}", module_section_osc, { 5, 0, 1, 2 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_glfo] = make_module_section_gui(
    "{96C75EE5-577E-4508-A85A-E92FF9FD8A4D}", module_section_glfo, { 3, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_gfx] = make_module_section_gui(
    "{654B206B-27AE-4DFD-B885-772A8AD0A4F3}", module_section_gfx, { 2, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_master_in] = make_module_section_gui(
    "{F9578AAA-66A4-4B0C-A941-4719B5F0E998}", module_section_master_in, { 1, 0, 1, 2 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_master_out] = make_module_section_gui(
    "{F77335AC-B701-40DA-B4C2-1F55DBCC29A4}", module_section_master_out, { 1, 2, 1, 1 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_voice_in] = make_module_section_gui(
    "{FB435C64-8349-4F0F-84FC-FFC82002D69F}", module_section_voice_in, { 4, 0, 1, 2 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_voice_out] = make_module_section_gui(
    "{2B764ECA-B745-4087-BB73-1B5952BC6B96}", module_section_voice_out, { 4, 2, 1, 1 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_monitor] = make_module_section_gui(
    "{8FDAEB21-8876-4A90-A8E1-95A96FB98FD8}", module_section_monitor, { 0, 1, 1, 1 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_matrices] = make_module_section_gui_tabbed(
    "{11A46FE6-9009-4C17-B177-467243E171C8}", module_section_matrices, { 1, 3, 7, 1 },
    { module_am_matrix, module_vaudio_matrix, module_gaudio_matrix, module_vcv_matrix, module_gcv_matrix });

  result->modules.resize(module_count);
  result->modules[module_midi] = midi_topo(module_section_hidden);
  result->modules[module_voice_mix] = voice_mix_topo(module_section_hidden);
  result->modules[module_voice_note] = voice_note_topo(module_section_hidden);
  result->modules[module_env] = env_topo(module_section_env, voice_colors, { 0, 0 });
  result->modules[module_gfx] = fx_topo(module_section_gfx, global_colors, { 0, 0 }, true);
  result->modules[module_vfx] = fx_topo(module_section_vfx, voice_colors, { 0, 0 }, false);
  result->modules[module_glfo] = lfo_topo(module_section_glfo, global_colors, { 0, 0 }, true);
  result->modules[module_vlfo] = lfo_topo(module_section_vlfo, voice_colors, { 0, 0 }, false);
  result->modules[module_osc] = osc_topo(module_section_osc, voice_colors, { 0, 0 });
  result->modules[module_master_in] = master_in_topo(module_section_master_in, global_colors, { 0, 0 });
  result->modules[module_voice_on_note] = voice_on_note_topo(result.get(), module_section_hidden); // must be after all global cv  
  result->modules[module_voice_in] = voice_in_topo(module_section_voice_in, voice_colors, { 0, 0 }); // must be after all cv
  result->modules[module_voice_out] = audio_out_topo(module_section_voice_out, voice_colors, { 0, 0 }, false);
  result->modules[module_master_out] = audio_out_topo(module_section_master_out, global_colors, { 0, 0 }, true);
  result->modules[module_monitor] = monitor_topo(module_section_monitor, monitor_colors, { 0, 0 }, result->polyphony);
  result->modules[module_am_matrix] = am_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, result.get());
  result->modules[module_gaudio_matrix] = audio_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, true,
    make_audio_matrix_sources(result.get(), true), make_audio_matrix_targets(result.get(), true));
  result->modules[module_vaudio_matrix] = audio_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, false,
    make_audio_matrix_sources(result.get(), false), make_audio_matrix_targets(result.get(), false));
  result->modules[module_gcv_matrix] = cv_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, true,
    make_cv_matrix_sources(result.get(), true), {}, make_cv_matrix_targets(result.get(), true));
  result->modules[module_vcv_matrix] = cv_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, false,
    make_cv_matrix_sources(result.get(), false),
    make_cv_matrix_sources(result.get(), true),
    make_cv_matrix_targets(result.get(), false));
  return result;
}

}