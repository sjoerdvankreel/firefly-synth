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

extern int const voice_in_param_mode;
extern int const master_in_param_midi_smooth;
extern int const master_in_param_tempo_smooth;

enum {
  custom_section_title,
  custom_section_main_graph,
  custom_section_controls,
  custom_section_gfx_graph,
  custom_section_glfo_graph,
  custom_section_matrix_graphs,
  custom_section_fx_count,
  custom_section_osc_graph = custom_section_fx_count,
  custom_section_vfx_graph,
  custom_section_vlfo_graph,
  custom_section_env_graph,
  custom_section_synth_count };

enum { 
  module_section_hidden, 
  module_section_master_in, module_section_master_out,
  module_section_gfx, module_section_glfo,
  module_section_monitor, module_section_matrices,
  module_section_fx_count,
  module_section_voice_in = module_section_fx_count, module_section_voice_out,
  module_section_osc, module_section_vfx, 
  module_section_vlfo, module_section_env, 
  module_section_synth_count };

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
  graph_params params;
  params.partition_scale = 0.15f;
  params.scale_type = graph_params::scale_w;
  module_graph_params module_params = make_module_graph_params(module, 
    render_on_module_mouse_enter, render_on_param_mouse_enter, dependent_module_indices);
  return store_component<module_graph>(store, gui, lnf, params, module_params);
}

static Component&
make_main_graph_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  graph_params params;
  params.partition_scale = 0.125f;
  params.scale_type = graph_params::scale_w;
  module_graph_params module_params;
  module_params.fps = 10;
  module_params.module_index = -1;
  module_params.render_on_tweak = true;
  module_params.render_on_tab_change = false;
  module_params.render_on_module_mouse_enter = true;
  module_params.render_on_param_mouse_enter_modules = {
    module_gcv_cv_matrix, module_master_in, module_vcv_cv_matrix, module_voice_in, module_osc_osc_matrix, 
    module_vaudio_audio_matrix, module_gaudio_audio_matrix, module_vcv_audio_matrix, module_gcv_audio_matrix };
  return store_component<module_graph>(store, gui, lnf, params, module_params);
}

static Component&
make_matrix_graphs_section(plugin_gui* gui, lnf* lnf, component_store store)
{
  return store_component<tabbed_module_section_container>(store, gui, module_section_matrices, 
    [gui, lnf](int module_index) -> std::unique_ptr<juce::Component> {
      graph_params params;
      params.partition_scale = 0.33f;
      params.scale_type = graph_params::scale_h;
      switch (module_index)
      {
      case module_osc_osc_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, true, false, { module_osc, module_voice_in } ));
      case module_vaudio_audio_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true, { }));
      case module_gaudio_audio_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true, { }));
      case module_vcv_audio_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true,
        { module_master_in, module_glfo, module_vlfo, module_env, module_voice_in, module_voice_out,
        module_osc, module_osc_osc_matrix, module_vfx, module_vaudio_audio_matrix }));
      case module_gcv_audio_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true,
        { module_master_in, module_glfo, module_gfx, module_gaudio_audio_matrix, module_master_out }));
      case module_vcv_cv_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true,
        { module_master_in, module_glfo, module_gcv_audio_matrix, module_vlfo, module_env, module_vcv_audio_matrix }));
      case module_gcv_cv_matrix: return std::make_unique<module_graph>(gui, lnf, params, make_module_graph_params(module_index, false, true,
        { module_master_in, module_glfo, module_gcv_audio_matrix }));
      default: assert(false); return std::make_unique<Component>();
      }
    });
}

static Component&
make_controls_section(plugin_gui* gui, lnf* lnf, bool is_fx, component_store store)
{
  std::vector<int> row_distribution;
  std::vector<int> column_distribution;
  if(is_fx) 
  {
    row_distribution = { 1 };
    column_distribution = { 100, 100, 100, 100, 100, 100, 145, 55, 100 };
  }
  else 
  {
    row_distribution = { 1, 1 };
    column_distribution = { 1, 1, 1, 1, 1 };
  }
  auto& result = store_component<grid_component>(store, gui_dimension{ row_distribution, column_distribution }, 2);
  result.add(gui->make_load_button(), { 0, 0 });
  result.add(gui->make_save_button(), { 0, 1 });
  result.add(gui->make_init_button(), { 0, 2 });
  result.add(gui->make_clear_button(), { 0, 3 });
  result.add(store_component<preset_button>(store, gui), { 0, 4 });
  auto& tweak_label = store_component<last_tweaked_label>(store, gui->gui_state());
  tweak_label.setJustificationType(Justification::centredRight);
  result.add(tweak_label, { is_fx? 0: 1, is_fx? 5: 0, 1, is_fx? 2: 3 });
  result.add(store_component<last_tweaked_editor>(store, gui->gui_state(), lnf), { is_fx? 0: 1, is_fx? 7: 3, 1, 2 });
  return result;
}

static Component&
make_title_section(plugin_gui* gui, lnf* lnf, component_store store, Colour const& color, bool is_fx)
{
  std::string name = is_fx? FF_SYNTH_FX_NAME: FF_SYNTH_INST_NAME;
  for(int i = 0; i < name.size(); i++) name[i] = std::toupper(name[i]);
  auto& grid = store_component<grid_component>(store, gui_dimension({ { 1 }, { gui_dimension::auto_size, 1 } }), 2, 0, 1);
  grid.add(store_component<image_component>(store, gui->gui_state()->desc().config, "firefly.png", RectanglePlacement::xRight), { 0, 1 });
  auto& label = store_component<autofit_label>(store, lnf, name, true, 15);
  label.setColour(Label::ColourIds::textColourId, color);
  grid.add(label, { 0, 0 });
  return grid;
}

std::unique_ptr<module_tab_menu_handler>
make_audio_routing_menu_handler(plugin_state* state, bool global, bool is_fx)
{
  auto cv_params = make_audio_routing_cv_params(state, global);
  auto audio_params = make_audio_routing_audio_params(state, global, is_fx);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params }));
}

cv_audio_matrix_mixdown
make_static_cv_matrix_mixdown(plugin_block& block)
{
  cv_audio_matrix_mixdown result;
  plugin_dims dims(block.plugin, block.plugin.audio_polyphony);
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
make_audio_audio_matrix_sources(plugin_topo const* topo, bool global, bool is_fx)
{
  if (!global) return { &topo->modules[module_osc], &topo->modules[module_vfx] };
  if (!is_fx) return { &topo->modules[module_voice_mix], &topo->modules[module_gfx] };
  return { &topo->modules[module_external_audio], &topo->modules[module_gfx] };
}

std::vector<module_topo const*>
make_audio_audio_matrix_targets(plugin_topo const* topo, bool global)
{
  if (global)
    return { &topo->modules[module_gfx], &topo->modules[module_master_out] };
  else
    return { &topo->modules[module_vfx], &topo->modules[module_voice_out] };
}

std::vector<module_topo const*>
make_cv_cv_matrix_targets(plugin_topo const* topo, bool global)
{
  if (global)
    return {
      &topo->modules[module_glfo],
      &topo->modules[module_gcv_audio_matrix] };
  else
    return {
      &topo->modules[module_vlfo],
      &topo->modules[module_vcv_audio_matrix] };
}

std::vector<module_topo const*>
make_cv_audio_matrix_targets(plugin_topo const* topo, bool global)
{
  if (global)
    return {
      &topo->modules[module_gfx], 
      &topo->modules[module_gaudio_audio_matrix],
      &topo->modules[module_master_out] };
  else
    return { 
      &topo->modules[module_voice_in],
      &topo->modules[module_osc], 
      &topo->modules[module_osc_osc_matrix],
      &topo->modules[module_vfx],
      &topo->modules[module_vaudio_audio_matrix],
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
synth_topo(bool is_fx)
{
  Colour voice_color(0xFFFF8844);
  Colour matrix_color(0xFF8888FF);
  Colour custom_color(0xFFFF4488);
  Colour global_color(0xFF4488FF);
  gui_colors voice_colors(make_section_colors(voice_color));
  gui_colors global_colors(make_section_colors(global_color));
  gui_colors matrix_colors(make_section_colors(matrix_color));
  gui_colors custom_colors(make_section_colors(custom_color));
  gui_colors monitor_colors(make_section_colors(custom_color));
  custom_colors.edit_text = custom_color;
  monitor_colors.control_text = custom_color;

  auto result = std::make_unique<plugin_topo>();
  result->graph_polyphony = 1;
  result->audio_polyphony = 32;
  result->extension = "ffpreset";
  result->vendor = "Sjoerd van Kreel";
  result->version_minor = FF_SYNTH_VERSION_MINOR;
  result->version_major = FF_SYNTH_VERSION_MAJOR;
  result->bpm_smooth_module = module_master_in;
  result->bpm_smooth_param = master_in_param_tempo_smooth;
  result->midi_smooth_module = module_master_in;
  result->midi_smooth_param = master_in_param_midi_smooth;
  result->voice_mode_module = module_voice_in;
  result->voice_mode_param = voice_in_param_mode;

  result->gui.aspect_ratio_width = 107;
  if(is_fx)
  {
    result->type = plugin_type::fx;
    result->tag.id = FF_SYNTH_FX_ID;
    result->tag.name = FF_SYNTH_FX_NAME;
    result->gui.min_width = 708;
    result->gui.aspect_ratio_height = 84;
  }
  else
  {
    result->type = plugin_type::synth;
    result->tag.id = FF_SYNTH_INST_ID;
    result->tag.name = FF_SYNTH_INST_NAME;
    result->gui.min_width = 1143;
    result->gui.aspect_ratio_height = 50;
  }

  // The same font takes more size on linux ?
#if (defined __linux__) || (defined  __FreeBSD__)
  result->gui.font_height = 11;
#endif

  result->gui.typeface_file_name = "Handel Gothic Regular.ttf";
  result->gui.dimension.column_sizes = { is_fx? 19: 17, is_fx? 28: 30, 10 };
  if(!is_fx) result->gui.dimension.column_sizes.push_back(35);
  int height = result->gui.min_width * result->gui.aspect_ratio_height / result->gui.aspect_ratio_width;
  std::vector<gui_vertical_section_size> section_vsizes = { { true, 1 }, { !is_fx, 1 }, { true, 1 }, { true, 1 }, { true, 1 } };
  if (!is_fx) section_vsizes.insert(section_vsizes.end(), { { true, 2 }, { true, 1 }, { true, 1 }, { true, 2 } });
  else section_vsizes.insert(section_vsizes.end(), { { true, 7 }, { true, 2 } });
  result->gui.dimension.row_sizes = gui_vertical_distribution(height, result->gui.font_height, section_vsizes);

  int section_voffset = is_fx? 1: 0;
  result->gui.custom_sections.resize(is_fx? custom_section_fx_count: custom_section_synth_count);
  auto make_title_section_ui = [custom_color, is_fx](plugin_gui* gui, lnf* lnf, auto store) -> Component& { 
    return make_title_section(gui, lnf, store, custom_color, is_fx); };
  result->gui.custom_sections[custom_section_title] = make_custom_section_gui(
    custom_section_title, { 0, 0, 1, 1 }, custom_colors, make_title_section_ui);
  result->gui.custom_sections[custom_section_controls] = make_custom_section_gui(
    custom_section_controls, { is_fx? 1: 0, is_fx? 0: 3, 1, is_fx? 3: 1 }, custom_colors,
      [is_fx](auto gui, auto lnf, auto store) -> juce::Component& { return make_controls_section(gui, lnf, is_fx, store); });
  result->gui.custom_sections[custom_section_main_graph] = make_custom_section_gui(
    custom_section_main_graph, { 0, 2, 1, 1 }, custom_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_main_graph_section(gui, lnf, store); });
  result->gui.custom_sections[custom_section_gfx_graph] = make_custom_section_gui(
    custom_section_gfx_graph, { section_voffset + 2, 2, 1, 1 }, global_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_gfx, false, false, {}); });
  result->gui.custom_sections[custom_section_glfo_graph] = make_custom_section_gui(
    custom_section_glfo_graph, { section_voffset + 3, 2, 1, 1 }, global_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_module_graph_section(gui, lnf, store, module_glfo, false, false, {}); });
  result->gui.custom_sections[custom_section_matrix_graphs] = make_custom_section_gui(
    custom_section_matrix_graphs, { is_fx? 6: 8, is_fx? 0: 3, 1, is_fx? 3: 1 }, matrix_colors, [](auto* gui, auto* lnf, auto store)
    -> Component& { return make_matrix_graphs_section(gui, lnf, store); });
  if(!is_fx)
  {
    result->gui.custom_sections[custom_section_osc_graph] = make_custom_section_gui(
      custom_section_osc_graph, { 5, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
      -> Component& { return make_module_graph_section(gui, lnf, store, module_osc, false, false, { module_osc_osc_matrix, module_voice_in }); });
    result->gui.custom_sections[custom_section_vfx_graph] = make_custom_section_gui(
      custom_section_vfx_graph, { 6, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
      -> Component& { return make_module_graph_section(gui, lnf, store, module_vfx, false, false, {}); });
    result->gui.custom_sections[custom_section_vlfo_graph] = make_custom_section_gui(
      custom_section_vlfo_graph, { 7, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
      -> Component& { return make_module_graph_section(gui, lnf, store, module_vlfo, false, false, {}); });
    result->gui.custom_sections[custom_section_env_graph] = make_custom_section_gui(
      custom_section_env_graph, { 8, 2, 1, 1 }, voice_colors, [](auto* gui, auto* lnf, auto store)
      -> Component& { return make_module_graph_section(gui, lnf, store, module_env, false, false, {}); });
  }

  result->gui.module_sections.resize(is_fx? module_section_fx_count: module_section_synth_count);
  result->gui.module_sections[module_section_hidden] = make_module_section_gui_none(
    "{F289D07F-0A00-4AB1-B87B-685CB4D8B2F8}", module_section_hidden);
  result->gui.module_sections[module_section_glfo] = make_module_section_gui(
    "{96C75EE5-577E-4508-A85A-E92FF9FD8A4D}", module_section_glfo, { section_voffset + 3, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_gfx] = make_module_section_gui(
    "{654B206B-27AE-4DFD-B885-772A8AD0A4F3}", module_section_gfx, { section_voffset + 2, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_master_in] = make_module_section_gui(
    "{F9578AAA-66A4-4B0C-A941-4719B5F0E998}", module_section_master_in, { section_voffset + 1, 0, 1, 2 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_master_out] = make_module_section_gui(
    "{F77335AC-B701-40DA-B4C2-1F55DBCC29A4}", module_section_master_out, { section_voffset + 1, 2, 1, 1 }, { { 1 }, { 1 } });
  result->gui.module_sections[module_section_monitor] = make_module_section_gui(
    "{8FDAEB21-8876-4A90-A8E1-95A96FB98FD8}", module_section_monitor, { 0, 1, 1, 1 }, { { 1 }, { 1 } });
  std::vector<int> matrix_modules = { module_osc_osc_matrix, module_vaudio_audio_matrix, module_gaudio_audio_matrix,
     module_vcv_audio_matrix, module_gcv_audio_matrix, module_vcv_cv_matrix, module_gcv_cv_matrix };
  if(is_fx) matrix_modules = { module_gaudio_audio_matrix, module_gcv_audio_matrix, module_gcv_cv_matrix };
  result->gui.module_sections[module_section_matrices] = make_module_section_gui_tabbed(
    "{11A46FE6-9009-4C17-B177-467243E171C8}", module_section_matrices, { is_fx? 5: 1, is_fx? 0: 3, is_fx? 1: 7, is_fx? 3: 1 }, matrix_modules);
  if (!is_fx)
  {
    result->gui.module_sections[module_section_vlfo] = make_module_section_gui(
      "{0DA0E7C3-8DBB-440E-8830-3B6087F23B81}", module_section_vlfo, { 7, 0, 1, 2 }, { 1, 1 });
    result->gui.module_sections[module_section_vfx] = make_module_section_gui(
      "{5BD5F320-3CA9-490B-B69E-36A003F41CEC}", module_section_vfx, { 6, 0, 1, 2 }, { 1, 1 });
    result->gui.module_sections[module_section_env] = make_module_section_gui(
      "{AB26F56E-DC6D-4F0B-845D-C750728F8FA2}", module_section_env, { 8, 0, 1, 2 }, { 1, 1 });
    result->gui.module_sections[module_section_osc] = make_module_section_gui(
      "{7A457CCC-E719-4C07-98B1-017EA7DEFB1F}", module_section_osc, { 5, 0, 1, 2 }, { { 1 }, { 1 } });
    result->gui.module_sections[module_section_voice_in] = make_module_section_gui(
      "{FB435C64-8349-4F0F-84FC-FFC82002D69F}", module_section_voice_in, { 4, 0, 1, 2 }, { { 1 }, { 1 } });
    result->gui.module_sections[module_section_voice_out] = make_module_section_gui(
      "{2B764ECA-B745-4087-BB73-1B5952BC6B96}", module_section_voice_out, { 4, 2, 1, 1 }, { { 1 }, { 1 } });
  }

  result->modules.resize(module_count);
  result->modules[module_midi] = midi_topo(module_section_hidden);
  result->modules[module_voice_note] = voice_note_topo(module_section_hidden);
  result->modules[module_voice_mix] = voice_mix_topo(module_section_hidden, is_fx);
  result->modules[module_external_audio] = external_audio_topo(module_section_hidden, is_fx);
  result->modules[module_env] = env_topo(is_fx? module_section_hidden: module_section_env, voice_colors, { 0, 0 });
  result->modules[module_gfx] = fx_topo(module_section_gfx, global_colors, { 0, 0 }, true, is_fx);
  result->modules[module_vfx] = fx_topo(is_fx ? module_section_hidden : module_section_vfx, voice_colors, { 0, 0 }, false, is_fx);
  result->modules[module_glfo] = lfo_topo(module_section_glfo, global_colors, { 0, 0 }, true);
  result->modules[module_vlfo] = lfo_topo(is_fx ? module_section_hidden : module_section_vlfo, voice_colors, { 0, 0 }, false);
  result->modules[module_osc] = osc_topo(is_fx ? module_section_hidden : module_section_osc, voice_colors, { 0, 0 });
  result->modules[module_master_in] = master_in_topo(module_section_master_in, is_fx, global_colors, { 0, 0 });
  result->modules[module_voice_on_note] = voice_on_note_topo(result.get(), module_section_hidden); // must be after all global cv  
  result->modules[module_voice_in] = voice_in_topo(is_fx ? module_section_hidden : module_section_voice_in, voice_colors, { 0, 0 }); // must be after all cv
  result->modules[module_voice_out] = audio_out_topo(is_fx ? module_section_hidden : module_section_voice_out, voice_colors, { 0, 0 }, false, is_fx);
  result->modules[module_master_out] = audio_out_topo(module_section_master_out, global_colors, { 0, 0 }, true, is_fx);
  result->modules[module_monitor] = monitor_topo(module_section_monitor, monitor_colors, { 0, 0 }, result->audio_polyphony, is_fx);
  result->modules[module_osc_osc_matrix] = osc_osc_matrix_topo(is_fx ? module_section_hidden : module_section_matrices, matrix_colors, { 0, 0 }, result.get());
  result->modules[module_gaudio_audio_matrix] = audio_audio_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, true, is_fx,
    make_audio_audio_matrix_sources(result.get(), true, is_fx), make_audio_audio_matrix_targets(result.get(), true));
  result->modules[module_vaudio_audio_matrix] = audio_audio_matrix_topo(is_fx ? module_section_hidden : module_section_matrices, matrix_colors, { 0, 0 }, false, is_fx,
    make_audio_audio_matrix_sources(result.get(), false, is_fx), make_audio_audio_matrix_targets(result.get(), false));
  result->modules[module_gcv_audio_matrix] = cv_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, false, true, is_fx,
    make_cv_matrix_sources(result.get(), true), {}, make_cv_audio_matrix_targets(result.get(), true));
  result->modules[module_vcv_audio_matrix] = cv_matrix_topo(is_fx ? module_section_hidden : module_section_matrices, matrix_colors, { 0, 0 }, false, false, is_fx,
    make_cv_matrix_sources(result.get(), false),
    make_cv_matrix_sources(result.get(), true),
    make_cv_audio_matrix_targets(result.get(), false));
  result->modules[module_gcv_cv_matrix] = cv_matrix_topo(module_section_matrices, matrix_colors, { 0, 0 }, true, true, is_fx,
    make_cv_matrix_sources(result.get(), true), {}, make_cv_cv_matrix_targets(result.get(), true));
  result->modules[module_vcv_cv_matrix] = cv_matrix_topo(is_fx ? module_section_hidden : module_section_matrices, matrix_colors, { 0, 0 }, true, false, is_fx,
    make_cv_matrix_sources(result.get(), false),
    make_cv_matrix_sources(result.get(), true),
    make_cv_cv_matrix_targets(result.get(), false));
  return result;
}

}