#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { section_main, section_pitch };
enum { param_mode, param_note, param_cent, param_count };

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{88F746C4-1A70-4A64-A11D-584D87D3059C}", "Poly");
  result.emplace_back("{BAC73AB6-BEBF-44DA-8D8B-AE7A9E31F62B}", "Poly Porta");
  result.emplace_back("{477EB0C5-66E5-40BA-B25B-357E6F92B653}", "Poly Porta Sync");
  result.emplace_back("{EDAFF7F8-7A52-45CE-BC68-12B2A3994EC6}", "Poly Porta Auto");
  result.emplace_back("{3A60F0FE-C792-4B9B-AD88-104553205377}", "Poly Porta Auto Sync");
  result.emplace_back("{6ABA8E48-F284-40A4-A0E2-C263B536D493}", "Mono Porta");
  result.emplace_back("{E9A634CE-BD35-4CA3-A0E1-613BF15E1218}", "Mono Porta Sync");
  result.emplace_back("{B381E0DE-8DC2-4304-AC28-93E4FCB866D7}", "Mono Porta Auto");
  result.emplace_back("{D968914B-E2AE-44A0-BCA7-F635D2AA81F7}", "Mono Porta Auto Sync");
  return result;
}

class voice_cv_engine :
public module_engine {
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_cv_engine);
};

module_topo
voice_cv_topo(int section, gui_colors const& colors, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{524138DF-1303-4961-915A-3CAABA69D53A}", "Voice", module_voice_cv, 1),
    make_module_dsp(module_stage::voice, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { { 1 }, { 1, 1 } } )));
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<voice_cv_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{C85AA7CC-FBD1-4631-BB7A-831A2E084E9E}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{F26D6913-63E8-4A23-97C0-9A17D859ED93}", "Mode", param_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));

  result.sections.emplace_back(make_param_section(section_pitch,
    make_topo_tag("{3EB05593-E649-4460-929C-993B6FB7BBD3}", "Pitch"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1 }))));
  
  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{CB6D7BC8-5DE6-4A84-97C9-4E405A96E0C8}", "Note", param_note, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_pitch, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  note.gui.submenu = make_midi_note_submenu();

  result.params.emplace_back(make_param(
    make_topo_info("{57A908CD-ED0A-4FCD-BA5F-92257175A9DE}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_pitch, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));

  return result;
}

}
