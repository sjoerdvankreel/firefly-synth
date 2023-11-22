#include <firefly_synth/synth.hpp>
#include <firefly_synth/plugin.hpp>

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/gui/controls.hpp>
#include <plugin_base/gui/containers.hpp>

using namespace juce;
using namespace plugin_base;

namespace firefly_synth {

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
  result.section_outline1 = c.darker(1);
  return result;
}

enum {
  custom_section_controls,
  custom_section_title,
  custom_section_count };

enum { 
  module_section_midi, module_section_input, 
  module_section_lfos, module_section_env, module_section_cv_matrix,
  module_section_audio_matrix, module_section_osc, module_section_fx, 
  module_section_voice, module_section_monitor_master, module_section_count };

static Component&
make_title_section(plugin_gui* gui, lnf* lnf, component_store store, Colour const& color)
{
  auto& grid = store_component<grid_component>(store, gui_dimension({ { 1 }, { 1, gui_dimension::auto_size } }), 2, 0, 1);
  grid.add(store_component<image_component>(store, gui->gui_state()->desc().config, "firefly.png", RectanglePlacement::xLeft), { 0, 0 });
  auto& label = store_component<autofit_label>(store, lnf, "FIREFLY SYNTH", true, 14);
  label.setColour(Label::ColourIds::textColourId, color);
  grid.add(label, { 0, 1 });
  return grid;
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

std::unique_ptr<plugin_topo>
synth_topo()
{
  Colour other_color(0xFFFF4488);
  gui_colors other_colors(make_section_colors(other_color));
  gui_colors monitor_colors(make_section_colors(other_color));
  gui_colors cv_colors(make_section_colors(Colour(0xFFFF8844)));
  gui_colors audio_colors(make_section_colors(Colour(0xFF8888FF)));
  other_colors.edit_text = other_color;
  monitor_colors.control_text = other_color;

  auto result = std::make_unique<plugin_topo>();
  result->polyphony = 32;
  result->extension = "ffpreset";
  result->vendor = "Sjoerd van Kreel";
  result->type = plugin_type::synth;
  result->tag.id = FF_SYNTH_ID;
  result->tag.name = FF_SYNTH_NAME;
  result->version_minor = FF_SYNTH_VERSION_MINOR;
  result->version_major = FF_SYNTH_VERSION_MAJOR;

  result->gui.min_width = 800;
  result->gui.aspect_ratio_width = 52;
  result->gui.aspect_ratio_height = 23;
  result->gui.typeface_file_name = "Handel Gothic Regular.ttf";
  result->gui.dimension.column_sizes = { 6, 14, 7, 7 };
  result->gui.dimension.row_sizes = { 1, 1, 1, 1, 1, 1, 1 };

  result->gui.custom_sections.resize(custom_section_count);
  auto make_title_section_ui = [other_color](plugin_gui* gui, lnf* lnf, auto store) -> Component& { 
    return make_title_section(gui, lnf, store, other_color); };
  result->gui.custom_sections[custom_section_title] = make_custom_section_gui(
    custom_section_title, { 0, 0, 1, 2 }, other_colors, make_title_section_ui);
  result->gui.custom_sections[custom_section_controls] = make_custom_section_gui(
    custom_section_controls, { 0, 2, 1, 2 }, other_colors, make_controls_section);

  result->gui.module_sections.resize(module_section_count);
  result->gui.module_sections[module_section_midi] = make_module_section_gui_none(
    "{F289D07F-0A00-4AB1-B87B-685CB4D8B2F8}", module_section_midi);
  result->gui.module_sections[module_section_voice] = make_module_section_gui_none(
    "{45767DB3-D1BE-4202-91B7-F6558F148D3D}", module_section_voice);
  result->gui.module_sections[module_section_fx] = make_module_section_gui(
    "{0DA0E7C3-8DBB-440E-8830-3B6087F23B81}", module_section_fx, { 5, 0, 1, 2 }, { 1, 2 });
  result->gui.module_sections[module_section_env] = make_module_section_gui(
    "{AB26F56E-DC6D-4F0B-845D-C750728F8FA2}", module_section_env, { 3, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_osc] = make_module_section_gui(
    "{7A457CCC-E719-4C07-98B1-017EA7DEFB1F}", module_section_osc, { 4, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_lfos] = make_module_section_gui(
    "{96C75EE5-577E-4508-A85A-E92FF9FD8A4D}", module_section_lfos, { 2, 0, 1, 2 }, { 1, 2 });
  result->gui.module_sections[module_section_input] = make_module_section_gui(
    "{F9578AAA-66A4-4B0C-A941-4719B5F0E998}", module_section_input, { 1, 0, 1, 2 }, { 1, 1 });
  result->gui.module_sections[module_section_monitor_master] = make_module_section_gui(
    "{8FDAEB21-8876-4A90-A8E1-95A96FB98FD8}", module_section_monitor_master, { 6, 0, 1, 2 }, { { 1 }, { 2, 1 } });
  result->gui.module_sections[module_section_cv_matrix] = make_module_section_gui_tabbed(
    "{11A46FE6-9009-4C17-B177-467243E171C8}", module_section_cv_matrix, { 1, 2, 3, 2 },
    "CV", result->gui.module_header_width, { module_vcv_matrix, module_gcv_matrix });
  result->gui.module_sections[module_section_audio_matrix] = make_module_section_gui_tabbed(
    "{950B6610-5CE1-4629-943F-CB2057CA7346}", module_section_audio_matrix, { 4, 2, 3, 2 },
    "Audio", result->gui.module_header_width, { module_vaudio_matrix, module_gaudio_matrix });

  result->modules.resize(module_count);
  result->modules[module_midi] = midi_topo(module_section_midi);
  result->modules[module_voice_in] = voice_topo(module_section_voice, false);
  result->modules[module_voice_out] = voice_topo(module_section_voice, true);
  result->modules[module_env] = env_topo(module_section_env, cv_colors, { 0, 0 });
  result->modules[module_osc] = osc_topo(module_section_osc, audio_colors, { 0, 0 });
  result->modules[module_gfx] = fx_topo(module_section_fx, audio_colors, { 0, 1 }, true);
  result->modules[module_vfx] = fx_topo(module_section_fx, audio_colors, { 0, 0 }, false);
  result->modules[module_glfo] = lfo_topo(module_section_lfos, cv_colors, { 0, 0 }, true);
  result->modules[module_vlfo] = lfo_topo(module_section_lfos, cv_colors, { 0, 1 }, false);
  result->modules[module_input] = input_topo(module_section_input, other_colors, { 0, 0 });
  result->modules[module_master] = master_topo(module_section_monitor_master, audio_colors, { 0, 1 });
  result->modules[module_monitor] = monitor_topo(module_section_monitor_master, monitor_colors, { 0, 0 }, result->polyphony);
  result->modules[module_vaudio_matrix] = audio_matrix_topo(module_section_audio_matrix, audio_colors, { 0, 0 }, false,
    { &result->modules[module_osc], &result->modules[module_vfx] },
    { &result->modules[module_vfx], &result->modules[module_voice_out] });
  result->modules[module_vcv_matrix] = cv_matrix_topo(module_section_cv_matrix, cv_colors, { 0, 0 }, false,
    { &result->modules[module_midi], &result->modules[module_input], &result->modules[module_glfo], &result->modules[module_vlfo], &result->modules[module_env] },
    { &result->modules[module_osc], &result->modules[module_vfx], &result->modules[module_vaudio_matrix] });
  result->modules[module_gaudio_matrix] = audio_matrix_topo(module_section_audio_matrix, audio_colors, { 0, 0 }, true,
    { &result->modules[module_voice_in], &result->modules[module_gfx] },
    { &result->modules[module_gfx], &result->modules[module_master] });
  result->modules[module_gcv_matrix] = cv_matrix_topo(module_section_cv_matrix, cv_colors, { 0, 0 }, true,
    { &result->modules[module_midi], &result->modules[module_input], &result->modules[module_glfo] },
    { &result->modules[module_gfx], &result->modules[module_gaudio_matrix], &result->modules[module_master] });
  return result;
}

}