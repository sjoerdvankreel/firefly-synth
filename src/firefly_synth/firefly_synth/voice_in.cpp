#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/shared/io_plugin.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { output_pitch_offset };
enum { over_1, over_2, over_4 };
enum { porta_off, porta_on, porta_auto };
enum { scratch_pb, scratch_cent, scratch_pitch, scratch_count };
enum { section_mode, section_oversmp, section_porta_sync, section_porta_note, section_uni_count, section_uni_prms };

enum {
  param_mode, param_oversmp, 
  param_porta_sync, param_porta, param_porta_time, param_porta_tempo,
  param_note, param_cent, 
  param_uni_voices, param_uni_dtn, param_uni_osc_phase, param_uni_lfo_dtn, param_uni_lfo_phase, param_uni_env_dtn, param_uni_sprd, 
  param_pitch, param_pb, param_count };

// we provide the buttons, everyone else needs to implement it
extern int const voice_in_param_mode = param_mode;
extern int const voice_in_param_oversmp = param_oversmp;
extern int const voice_in_output_pitch_offset = output_pitch_offset;
extern int const voice_in_param_uni_dtn = param_uni_dtn;
extern int const voice_in_param_uni_sprd = param_uni_sprd;
extern int const voice_in_param_uni_voices = param_uni_voices;
extern int const voice_in_param_uni_env_dtn = param_uni_env_dtn;
extern int const voice_in_param_uni_lfo_dtn = param_uni_lfo_dtn;
extern int const voice_in_param_uni_lfo_phase = param_uni_lfo_phase;
extern int const voice_in_param_uni_osc_phase = param_uni_osc_phase;

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{88F746C4-1A70-4A64-A11D-584D87D3059C}", "Polyphonic");
  result.emplace_back("{6ABA8E48-F284-40A4-A0E2-C263B536D493}", "Monophonic");
  result.emplace_back("{519341B0-4F79-4433-9449-1386F927E88B}", "Release Mono");
  return result;
}

static std::vector<list_item>
porta_items()
{
  std::vector<list_item> result;
  result.emplace_back("{51C360E5-967A-4218-B375-5052DAC4FD02}", "Off");
  result.emplace_back("{112A9728-8564-469E-95A7-34FE5CC7C8FC}", "On", "On (Constant Pitch)");
  result.emplace_back("{0E3AF80A-F242-4176-8C72-C0C91D72AEBB}", "Auto", "Auto (Constant Time)");
  return result;
}

static std::vector<list_item>
over_items()
{
  std::vector<list_item> result;
  result.emplace_back("{F9C54B64-3635-417F-86A9-69B439548F3C}", "1X");
  result.emplace_back("{937686E8-AC03-420B-A3FF-0ECE1FF9B23E}", "2X");
  result.emplace_back("{64F2A767-DE91-41DF-B2F1-003FCC846384}", "4X");
  return result;
}

class voice_in_state_converter :
public state_converter
{
  plugin_desc const* const _desc;
public:
  voice_in_state_converter(plugin_desc const* const desc) : _desc(desc) {}
  void post_process_always(load_handler const& handler, plugin_state& new_state) override;
  void post_process_existing(load_handler const& handler, plugin_state& new_state) override {}

  bool handle_invalid_param_value(
    std::string const& new_module_id, int new_module_slot,
    std::string const& new_param_id, int new_param_slot,
    std::string const& old_value, load_handler const& handler,
    plain_value& new_value) override;
};

class voice_in_engine :
public module_engine {
  int _position = -1;
  int _porta_samples = -1;
  float _to_midi_note = -1;
  float _from_midi_note = -1;
  float _mono_porta_time = -1;
  int _mono_porta_samples = -1;
  bool _first_note_in_mono_section = true;
  bool _mono_section_finished_in_voice = false; // for release mode
  
  float calc_current_porta_midi_note();
  template <engine_voice_mode VoiceMode>
  void process_voice_mode(plugin_block& block);
  template <engine_voice_mode VoiceMode, engine_tuning_mode TuningMode>
  void process_voice_mode_tuning_mode(plugin_block& block);
  template <engine_voice_mode VoiceMode, engine_tuning_mode TuningMode, bool GlobalUnison>
  void process_voice_mode_tuning_mode_unison(plugin_block& block);

public:
  void reset_audio(plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_in_engine);
};

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param, 
  param_topo_mapping const& mapping, std::vector<mod_out_custom_state> const& custom_outputs)
{
  if (mapping.param_index == param_uni_voices)
    return graph_data(graph_data_type::na, {});
  bool bipolar = mapping.param_index == param_cent;
  std::string partition = state.desc().params[param]->info.name;
  if (mapping.param_index == param_cent || mapping.param_index == param_porta_time)
    return graph_data(state.get_plain_at(mapping).real(), bipolar, { partition });
  return graph_data(graph_data_type::na, {});
}

bool
voice_in_state_converter::handle_invalid_param_value(
  std::string const& new_module_id, int new_module_slot,
  std::string const& new_param_id, int new_param_slot,
  std::string const& old_value, load_handler const& handler,
  plain_value& new_value)
{
  // Max osc oversampling got reduced from 8x to 4x.
  if (handler.old_version() < plugin_version{ 1, 7, 2 })
    if (new_param_id == _desc->plugin->modules[module_voice_in].params[param_oversmp].info.tag.id)
      if (old_value == "{9C6E560D-4999-40D9-85E4-C02468296206}")
      {
        new_value = _desc->raw_to_plain_at(module_voice_in, param_oversmp, over_4);
        return true;
      }

  return false;
}

// Global unison params got moved from global in to voice in.
void
voice_in_state_converter::post_process_always(load_handler const& handler, plugin_state& new_state)
{
  std::string old_value;
  auto const& modules = new_state.desc().plugin->modules;
  std::string global_in_id = modules[module_global_in].info.tag.id;

  // All global uni params moved from Global-In to Voice-In.
  if (handler.old_version() < plugin_version{ 1, 9, 0 })
  {
    if (handler.old_param_value(global_in_id, 0, "{C2B06E63-0283-4564-BABB-F20D9B30AD68}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_voices, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{2F0E199D-7B8A-497E-BED4-BC0FC55F1720}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_dtn, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{35D94C8A-3986-44EC-A4D6-485ACF199C4C}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_osc_phase, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{1B61F48D-7995-4295-A8DB-3AA44E1BF346}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_lfo_dtn, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{1799D722-B551-485F-A7F1-0590D97514EF}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_lfo_phase, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{52E0A939-296F-4F2A-A1E4-F283556B0BFD}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_env_dtn, 0, old_value);
    if (handler.old_param_value(global_in_id, 0, "{356468BC-59A0-40D0-AC14-C7DDBB16F4CE}", 0, old_value))
      new_state.set_text_at(module_voice_in, 0, param_uni_sprd, 0, old_value);
  }
}

module_topo
voice_in_topo(int section, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info_basic("{524138DF-1303-4961-915A-3CAABA69D53A}", "Voice", module_voice_in, 1),
    make_module_dsp(module_stage::voice, module_output::cv, scratch_count, {
      make_module_dsp_output(false, -1, make_topo_info_basic("{58E73C3A-CACD-48CC-A2B6-25861EC7C828}", "Pitch", 0, 1)) }),
      make_module_gui(section, pos, { { 1, 1 }, { 32, 8, 39, 13, 50 } })));
  result.info.description = "Oscillator common module. Controls portamento, oversampling and base pitch for all oscillators. Also contains global unison support.";

  result.graph_renderer = render_graph;
  result.gui.force_rerender_graph_on_param_hover = true;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<voice_in_engine>(); };
  result.state_converter_factory = [](auto desc) { return std::make_unique<voice_in_state_converter>(desc); };

  auto& mode_section = result.sections.emplace_back(make_param_section(section_mode,
    make_topo_tag_basic("{C85AA7CC-FBD1-4631-BB7A-831A2E084E9E}", "Mode"),
    make_param_section_gui({ 0, 0, 1, 1 }, gui_dimension({ { 1 } }, { { 1 } }))));
  mode_section.gui.merge_with_section = section_oversmp;
  auto& voice_mode = result.params.emplace_back(make_param(
    make_topo_info("{F26D6913-63E8-4A23-97C0-9A17D859ED93}", true, "Voice Mode", "Mode", "Mode", param_mode, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_mode, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  voice_mode.info.description = std::string("Selects poly/mono mode.<br/>") +
    "Poly - regular polyphonic mode.<br/>" +
    "Mono - true monophonic mode, may cause clicks.<br/>" +
    "Release - monophonic untill a mono section is released. So, multiple mono sections may overlap.<br/>"
    "To avoid clicks it is best to use release-monophonic mode with multi-triggered envelopes.";

  auto& ovrsmp_section = result.sections.emplace_back(make_param_section(section_oversmp,
    make_topo_tag_basic("{A61A30E3-D18C-4064-9B68-9CE8D55A7940}", "Oversampling"),
    make_param_section_gui({ 1, 0, 1, 1 }, gui_dimension({ { 1 } }, { { 1 } }))));
  ovrsmp_section.gui.merge_with_section = section_mode;
  auto& oversmp = result.params.emplace_back(make_param(
    make_topo_info("{0A866D59-E7C1-4D45-9DAF-D0C62EA03E93}", true, "Osc Oversampling", "Osc OvrSmp", "Osc OvrSmp", param_oversmp, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(over_items(), ""),
    make_param_gui_single(section_oversmp, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  oversmp.info.description = std::string("Oversampling for those rare cases where it makes a positive difference. ") +
    "Only affects FM and hardsync, but not AM. " +
    "Oversampling is per osc unison voice, so setting both this and osc unison to 4 results in an oscillator being 16 times as expensive to calculate. "  + 
    "Then multiply that by global unison.";

  auto& porta_sync_section = result.sections.emplace_back(make_param_section(section_porta_sync,
    make_topo_tag_basic("{11E4DE4C-A824-424E-BC5E-014240518C0F}", "Sync"),
    make_param_section_gui({ 0, 1, 2, 1 }, gui_dimension({ { 1, 1 }, { { 1 } } }), gui_label_edit_cell_split::vertical)));
  porta_sync_section.gui.merge_with_section = section_porta_note;
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{FE70E21D-2104-4EB6-B852-6CD9690E5F72}", true, "Porta Tempo Sync", "Sync", "Sync", param_porta_sync, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_porta_sync, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::top, gui_label_justify::center))));
  sync.gui.bindings.enabled.bind_params({ param_porta }, [](auto const& vs) { return vs[0] != porta_off; });
  sync.info.description = "Selects time or tempo-synced mode.";

  auto& porta_note_section = result.sections.emplace_back(make_param_section(section_porta_note,
    make_topo_tag_basic("{1C5D7493-AD1C-4F89-BF32-2D0092CB59EF}", "Mid"),
    make_param_section_gui({ 0, 2, 2, 1 }, gui_dimension({ { 1, 1 }, { { gui_dimension::auto_size_all, 1 } } }))));
  porta_note_section.gui.merge_with_section = section_porta_sync;
  auto& porta = result.params.emplace_back(make_param(
    make_topo_info("{586BEE16-430A-483E-891B-48E89C4B8FC1}", true, "Porta Mode", "Porta", "Porta", param_porta, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(porta_items(), ""),
    make_param_gui_single(section_porta_note, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  porta.info.description = std::string("Selects portamento mode.<br/>") +
    "Off - no portamento.<br/>" +
    "On - glides 1 semitone in the specified time, so glide pitch is constant and glide time is variable.<br/>" +
    "Auto - glides pitch difference between old and new note in the specified time, so glide pitch is variable and glide time is constant.";
  auto& time = result.params.emplace_back(make_param(
    make_topo_info("{E8301E86-B6EE-4F87-8181-959A05384866}", true, "Porta Time", "Time", "Time", param_porta_time, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_log(0.001, 10, 0.1, 1, 3, "Sec"),
    make_param_gui_single(section_porta_note, gui_edit_type::hslider, { 0, 1 }, make_label_none())));
  time.gui.bindings.enabled.bind_params({ param_porta }, [](auto const& vs) { return vs[0] != porta_off; });
  time.gui.bindings.visible.bind_params({ param_porta, param_porta_sync }, [](auto const& vs) { return vs[1] == 0; });
  time.info.description = "Pitch glide time in seconds.";
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info("{15271CBC-9876-48EC-BD3C-480FF68F9ACC}", true, "Porta Tempo", "Tempo", "Tempo", param_porta_tempo, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_timesig_default(false, { 4, 1 }, { 1, 16 }),
    make_param_gui_single(section_porta_note, gui_edit_type::list, { 0, 1 }, make_label_none())));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_porta }, [](auto const& vs) { return vs[0] != porta_off; });
  tempo.gui.bindings.visible.bind_params({ param_porta, param_porta_sync }, [](auto const& vs) { return vs[1] == 1; });
  tempo.info.description = "Pitch glide time in bars.";
  auto& note = result.params.emplace_back(make_param(
    make_topo_info_basic("{CB6D7BC8-5DE6-4A84-97C9-4E405A96E0C8}", "Note", param_note, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_porta_note, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  note.gui.submenu = make_midi_note_submenu();
  note.info.description = "Oscillator base pitch adjustment for all Oscs, C4 is no adjustment.";
  auto& cent = result.params.emplace_back(make_param(
    make_topo_info_basic("{57A908CD-ED0A-4FCD-BA5F-92257175A9DE}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_porta_note, gui_edit_type::hslider, { 1, 1 },
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::near))));
  cent.info.description = "Oscillator pitch cents adjustment for all Oscs.";

  auto& uni_count = result.sections.emplace_back(make_param_section(section_uni_count,
    make_topo_tag_basic("{550AAF78-C95A-4D4E-814C-0C5CC26C6457}", "Unison Voices"),
    make_param_section_gui({ 0, 3, 2, 1 }, gui_dimension({ 1, 1 }, { 1 }), gui_label_edit_cell_split::vertical)));
  uni_count.gui.merge_with_section = section_uni_prms;
  auto& uni_voices = result.params.emplace_back(make_param(
    make_topo_info("{C2B06E63-0283-4564-BABB-F20D9B30AD68}", true, "Global Unison Voices", "Unison", "Uni", param_uni_voices, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, max_global_unison_voices, 1, 0),
    make_param_gui_single(section_uni_count, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::top, gui_label_justify::center))));
  uni_voices.info.description = "Global unison voice count. Global unison spawns an entire polyphonic synth voice per unison voice. This includes per-voice oscillators, effects, lfo's and envelopes.";
  uni_voices.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly; });

  auto& uni_params = result.sections.emplace_back(make_param_section(section_uni_prms,
    make_topo_tag_basic("{7DCA43C8-CD48-4414-9017-EC1B982281FF}", "Global Unison Params"),
    make_param_section_gui({ 0, 4, 2, 1 }, gui_dimension({ 1, 1 }, {
      gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1 }),
      gui_label_edit_cell_split::horizontal)));
  uni_params.gui.merge_with_section = section_uni_count;
  auto& uni_dtn = result.params.emplace_back(make_param(
    make_topo_info("{2F0E199D-7B8A-497E-BED4-BC0FC55F1720}", true, "Global Unison Osc Detune", "Osc Dtn", "Uni Osc Dtn", param_uni_dtn, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.33, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_dtn.info.description = "Global unison voice pitch detune amount.";
  uni_dtn.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });
  auto& uni_osc_phase = result.params.emplace_back(make_param(
    make_topo_info("{35D94C8A-3986-44EC-A4D6-485ACF199C4C}", true, "Global Unison Osc Phase Offset", "Osc Phs", "Uni Osc Phs", param_uni_osc_phase, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.0, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_osc_phase.info.description = "Global unison voice osc phase offset.";
  uni_osc_phase.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });
  auto& uni_lfo_dtn = result.params.emplace_back(make_param(
    make_topo_info("{1B61F48D-7995-4295-A8DB-3AA44E1BF346}", true, "Global Unison LFO Detune", "LFO Dtn", "Uni LFO Dtn", param_uni_lfo_dtn, 1),
    make_param_dsp_accurate(param_automate::automate), make_domain_percentage_identity(0.0, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_lfo_dtn.info.description = "Global unison voice LFO detune amount.";
  uni_lfo_dtn.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });
  auto& uni_lfo_phase = result.params.emplace_back(make_param(
    make_topo_info("{1799D722-B551-485F-A7F1-0590D97514EF}", true, "Global Unison LFO Phase Offset", "LFO Phs", "Uni LFO Phs", param_uni_lfo_phase, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_percentage_identity(0.0, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_lfo_phase.info.description = "Global unison voice LFO phase offset.";
  uni_lfo_phase.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });
  auto& uni_env_dtn = result.params.emplace_back(make_param(
    make_topo_info("{52E0A939-296F-4F2A-A1E4-F283556B0BFD}", true, "Global Unison Env Detune", "Env Dtn", "Uni Env Dtn", param_uni_env_dtn, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_percentage_identity(0.0, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_env_dtn.info.description = "Global unison voice envelope detune amount.";
  uni_env_dtn.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });
  auto& uni_spread = result.params.emplace_back(make_param(
    make_topo_info("{356468BC-59A0-40D0-AC14-C7DDBB16F4CE}", true, "Global Unison Spread", "Spread", "Uni Spread", param_uni_sprd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_uni_prms, gui_edit_type::knob, { 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  uni_spread.info.description = "Global unison stereo spread.";
  uni_spread.gui.bindings.enabled.bind_params({ param_mode, param_uni_voices }, [](auto const& vs) { return vs[0] == engine_voice_mode_poly && vs[1] > 1; });

  auto& pitch = result.params.emplace_back(make_param(
    make_topo_info_basic("{034AE513-9AB6-46EE-8246-F6ECCC11CAE0}", "Pitch", param_pitch, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-128, 128, 0, 0, ""),
    make_param_gui_none()));
  pitch.info.description = "Absolute pitch modulation target for all Oscs.";
  auto& pb = result.params.emplace_back(make_param(
    make_topo_info("{BF20BA77-A162-401B-9F32-92AE34841AB2}", true, "Pitch Bend", "Pitch Bend", "Pitch Bend", param_pb, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_none()));
  pb.info.description = "Pitch-bend modulation target for all Oscs. Reacts to global pitchbend range.";
  return result;  
}

inline float
voice_in_engine::calc_current_porta_midi_note()
{
  if (_porta_samples == 0) return _to_midi_note;
  float range = _to_midi_note - _from_midi_note;
  float result = _from_midi_note + (_position / (float)_porta_samples * range);
  assert(!std::isnan(result));
  return result;
}

void
voice_in_engine::reset_audio(
  plugin_block const* block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  _position = 0;
  _to_midi_note = block->voice->state.note_id_.key;
  _from_midi_note = block->voice->state.note_id_.key;
  if (block->voice->state.note_id_.channel == block->voice->state.last_note_channel)
    _from_midi_note = block->voice->state.last_note_key;

  auto const& block_auto = block->state.own_block_automation;
  int porta_mode = block_auto[param_porta][0].step();
  bool porta_sync = block_auto[param_porta_sync][0].step() != 0;
  float porta_time_time = block_auto[param_porta_time][0].real();
  float porta_time_tempo = get_timesig_time_value(*block, block->host.bpm, module_voice_in, param_porta_tempo);
  float porta_time = porta_sync? porta_time_tempo: porta_time_time;
  switch (porta_mode)
  {
  case porta_off: _porta_samples = 0; break;
  case porta_auto: _porta_samples = porta_time * block->sample_rate; break;
  case porta_on: _porta_samples = porta_time * block->sample_rate * std::abs(_from_midi_note - _to_midi_note); break;
  default: assert(false); break;
  }

  // in monophonic mode we do not glide the first note in 
  // monophonic section. instead the first note is "plain"
  // and all subsequent notes in mono section apply portamento
  if(block_auto[param_mode][0].step() != engine_voice_mode_poly)
  {
    _from_midi_note = _to_midi_note;
    _mono_porta_time = porta_time;
    _mono_porta_samples = _porta_samples;
    _porta_samples = 0;
  }

  // reset mono section info
  _first_note_in_mono_section = true;
  _mono_section_finished_in_voice = false;
}

void
voice_in_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  auto const& block_auto = block.state.own_block_automation;
  int voice_mode = block_auto[param_mode][0].step();
  switch (voice_mode)
  {
  case engine_voice_mode_poly:
    process_voice_mode<engine_voice_mode_poly>(block);
    break;
  case engine_voice_mode_mono:
    process_voice_mode<engine_voice_mode_mono>(block);
    break;
  case engine_voice_mode_release:
    process_voice_mode<engine_voice_mode_release>(block);
    break;
  default:
    assert(false);
    break;
  }
}

template <engine_voice_mode VoiceMode> void
voice_in_engine::process_voice_mode(plugin_block& block)
{
  switch (block.current_tuning_mode)
  {
  case engine_tuning_mode_no_tuning:
    process_voice_mode_tuning_mode<VoiceMode, engine_tuning_mode_no_tuning>(block);
    break;
  case engine_tuning_mode_on_note_before_mod:
    process_voice_mode_tuning_mode<VoiceMode, engine_tuning_mode_on_note_before_mod>(block);
    break;
  case engine_tuning_mode_on_note_after_mod:
    process_voice_mode_tuning_mode<VoiceMode, engine_tuning_mode_on_note_after_mod>(block);
    break;
  case engine_tuning_mode_continuous_before_mod:
    process_voice_mode_tuning_mode<VoiceMode, engine_tuning_mode_continuous_before_mod>(block);
    break;
  case engine_tuning_mode_continuous_after_mod:
    process_voice_mode_tuning_mode<VoiceMode, engine_tuning_mode_continuous_after_mod>(block);
    break;
  default:
    assert(false);
    break;
  }
}

template <engine_voice_mode VoiceMode, engine_tuning_mode TuningMode> void
voice_in_engine::process_voice_mode_tuning_mode(plugin_block& block)
{
  if (block.voice->state.sub_voice_count > 1)
    process_voice_mode_tuning_mode_unison<VoiceMode, TuningMode, true>(block);
  else
    process_voice_mode_tuning_mode_unison<VoiceMode, TuningMode, false>(block);
}

template <engine_voice_mode VoiceMode, engine_tuning_mode TuningMode, bool GlobalUnison> void
voice_in_engine::process_voice_mode_tuning_mode_unison(plugin_block& block)
{  
  auto const& block_auto = block.state.own_block_automation;
  int note = block_auto[param_note][0].step();
  int porta_mode = block_auto[param_porta][0].step();

  auto const& modulation = get_cv_audio_matrix_mixdown(block, false);
  int global_pb_range = block.state.all_block_automation[module_global_in][0][global_in_param_pb_range][0].step();
  auto const& glob_uni_dtn_curve = *(modulation)[module_voice_in][0][voice_in_param_uni_dtn][0];

  auto const& pb_curve_norm = *(modulation)[module_voice_in][0][param_pb][0];
  auto& pb_curve = block.state.own_scratch[scratch_pb];
  block.normalized_to_raw_block<domain_type::linear>(module_voice_in, param_pb, pb_curve_norm, pb_curve);

  auto const& cent_curve_norm = *(modulation)[module_voice_in][0][param_cent][0];
  auto& cent_curve = block.state.own_scratch[scratch_cent];
  block.normalized_to_raw_block<domain_type::linear>(module_voice_in, param_cent, cent_curve_norm, cent_curve);

  auto const& pitch_curve_norm = *(modulation)[module_voice_in][0][param_pitch][0];
  auto& pitch_curve = block.state.own_scratch[scratch_pitch];
  block.normalized_to_raw_block<domain_type::linear>(module_voice_in, param_pitch, pitch_curve_norm, pitch_curve);
  
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if constexpr (VoiceMode != engine_voice_mode_poly)
    {
      if (block.state.mono_note_stream[f].event_type == mono_note_stream_event::off && VoiceMode == engine_voice_mode_release)
        _mono_section_finished_in_voice = true;
      if (!_mono_section_finished_in_voice && block.state.mono_note_stream[f].event_type == mono_note_stream_event::on)
      {
        if (porta_mode == porta_off)
        {
          // pitch switch, will be picked up by the oscs
          _position = 0;
          _porta_samples = 0;
          _to_midi_note = block.state.mono_note_stream[f].midi_key;
          _from_midi_note = _to_midi_note;
        }
        else
        {
          // start a new porta section within the current voice
          _from_midi_note = calc_current_porta_midi_note();
          _to_midi_note = block.state.mono_note_stream[f].midi_key;
          if (_first_note_in_mono_section)
          {
            _from_midi_note = _to_midi_note;
            _first_note_in_mono_section = false;
          }
          _position = 0;

          // need to recalc total glide time
          if (porta_mode == porta_on)
            _porta_samples = _mono_porta_time * block.sample_rate * std::abs(_from_midi_note - _to_midi_note);
          else
            _porta_samples = _mono_porta_samples;
        }
      }
    }

    // global unison detuning for this voice
    float glob_uni_detune = 0;
    if constexpr (VoiceMode == engine_voice_mode_poly && GlobalUnison)
    {
      float voice_pos = (float)block.voice->state.sub_voice_index / (block.voice->state.sub_voice_count - 1.0f);
      glob_uni_detune = (voice_pos - 0.5f) * glob_uni_dtn_curve[f];
    }

    float porta_note = 0;
    if (_position == _porta_samples) porta_note = _to_midi_note;
    else
    {
      porta_note = calc_current_porta_midi_note();
      _position++;
    }

    float new_pitch_offset = note + cent_curve[f] + glob_uni_detune - midi_middle_c;
    new_pitch_offset += porta_note - midi_middle_c;
    new_pitch_offset += pitch_curve[f] + pb_curve[f] * global_pb_range;

    // microtuning support
    if constexpr (TuningMode == engine_tuning_mode_on_note_before_mod || TuningMode == engine_tuning_mode_continuous_before_mod)
    {
      // be sure to NOT take the midi note that started the voice here
      // _to_midi_note takes voice recycling into account for mono mode
      float retuning_offset = _to_midi_note - (*block.current_tuning)[_to_midi_note].retuned_semis;
      new_pitch_offset -= retuning_offset;
    }

    block.state.own_cv[output_pitch_offset][0][f] = new_pitch_offset;
  }
}

}
