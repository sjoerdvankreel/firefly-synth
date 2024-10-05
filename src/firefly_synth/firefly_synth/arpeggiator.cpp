#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>

#include <cmath>
#include <array>
#include <random>
#include <algorithm>

using namespace plugin_base;
 
namespace firefly_synth {

static float const mod_range_exp = 4.0f;
static float const mod_range_linear = 8.0f;
static int const max_active_table_size = 256;

enum { section_table, section_notes, section_sample };
enum { output_abs_pos, output_rel_pos, output_abs_note, output_rel_note };

enum {
  param_type, param_jump,
  param_mode, param_flip, param_seed,
  param_notes, param_dist,
  param_rate_hz, param_rate_tempo, param_rate_mod_rate_hz, param_rate_mod_rate_tempo, param_sync,
  param_rate_mod_type, param_rate_mod_mode, param_rate_mod_amt };

enum { 
  mode_up, mode_down,
  mode_up_down1, mode_up_down2,
  mode_down_up1, mode_down_up2,
  mode_rand, mode_fixrand,
  mode_rand_free, mode_fixrand_free };

enum {
  mod_mode_off, mod_mode_linear, mod_mode_exp };

enum { 
  type_off, type_straight, 
  type_p1_oct, type_p2_oct, type_p3_oct, 
  type_m1p1_oct, type_m2p2_oct, type_m3p3_oct };

static bool 
is_random(int mode)
{
  return mode == mode_rand || mode == mode_fixrand || 
    mode == mode_rand_free || mode == mode_fixrand_free;
}

// what the user is doing eg press ceg -> those are on, release e -> that one's off
struct arp_user_note
{
  bool on = {};
  float velocity = {};
};

// what the active table looks like eg press ceg with type = +1 oct, mode = up -> c4e4g4c5e5g5
struct arp_table_note
{
  int midi_key;
  float velocity;
};

static std::vector<list_item>
mod_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{3FDB85A8-1127-475E-AE04-194BE3154C1C}", "Off", "Off");
  result.emplace_back("{E341B6D4-F0EA-4521-81CA-34F489224824}", "Linear", "Equal space between note lengths (e.g. 1, 2, 3, 4).");
  result.emplace_back("{2856FBED-8720-4BE9-945C-1DD7092B8D6F}", "Exp", "Exponential space between note lengths (e.g 1, 2, 4, 8).");
  return result;
}

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{70109417-1525-48A6-AE1D-7AB0E5765310}", "Off", "Off");
  result.emplace_back("{85A091D9-1283-4E67-961E-48C57BC68EB7}", "Plain", "Chord plain");
  result.emplace_back("{20CDFF90-9D2D-4AFD-8138-1BCB61370F23}", "+1 Oct", "Chord + 1 octave up");
  result.emplace_back("{4935E002-3745-4097-887F-C8ED52213658}", "+2 Oct", "Chord + 2 octaves up");
  result.emplace_back("{DCC943F4-5447-413F-B741-A83F2B84C259}", "+3 Oct", "Chord + 3 octaves up");
  result.emplace_back("{018FFDF0-AA2F-40BB-B449-34C8E93BCEB2}", "+/- 1", "Chord + 1 octave up/down");
  result.emplace_back("{E802C511-30E0-4B9B-A548-173D4C807AFF}", "+/- 2", "Chord + 2 octaves up/down");
  result.emplace_back("{7EA59B15-D0AE-4B6F-8A04-0AA4630F5E39}", "+/- 3", "Chord + 3 octaves up/down");
  return result;
}

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{25F4EF71-60E4-4F60-B613-8549C1BA074B}", "Up", "Low to high");
  result.emplace_back("{1772EDDE-6EC2-4F72-AC98-5B521AFB0EF1}", "Down", "High to low");
  result.emplace_back("{1ECA59EC-B4B5-4EE9-A1E8-0169E7F32BCC}", "UpDn 1", "Low to high to low, don't repeat first and last");
  result.emplace_back("{EB5FC7ED-DFA3-4DD5-A5F3-444469FDFBCF}", "UpDn 2", "Low to high to low, repeat first and last");
  result.emplace_back("{B48727A2-E886-43D4-906D-D87F5E7EE3CD}", "DnUp 1", "High to low to high, don't repeat first and last");
  result.emplace_back("{86488834-DB23-4467-8EB6-4C4261989233}", "DnUp 2", "High to low to high, repeat first and last");
  result.emplace_back("{C9095A2C-3F11-4C4B-A428-6DC948DFDB2C}", "Rnd", "New pattern on chord, fix pattern on repeat");
  result.emplace_back("{05A6B86C-1DCC-41F0-BE03-03B7D932FE5B}", "FixRnd", "Fix pattern on chord, fix pattern on repeat");
  result.emplace_back("{DC858447-2FB6-4081-BE94-4C3F9FB835EB}", "RndFree", "New pattern on chord, new pattern on repeat");
  result.emplace_back("{39619505-DD48-4B91-92F4-5CDDBECC8872}", "FixFree", "Fix pattern on chord, new pattern on repeat");
  return result;
}

class arpeggiator_engine :
public module_engine
{
  int _prev_type = -1;
  int _prev_mode = -1;
  int _prev_flip = -1;
  int _prev_seed = -1;
  int _prev_dist = -1;
  int _prev_sync = -1;
  int _prev_jump = -1;
  int _prev_notes = -1;

  int _table_pos = -1;
  int _note_remaining = 0;
  float _mod_phase = 0.0f; // internal lfo
  float _cv_out_abs_pos = 0.0f;
  float _cv_out_rel_pos = 0.0f;
  float _cv_out_abs_note = 0.0f;
  float _cv_out_rel_note = 0.0f;
  std::default_random_engine _random = {};

  int _note_low_key = -1;
  int _note_high_key = -1;
  std::array<int, 128> _note_abs_mapping = {};
  std::array<arp_user_note, 128> _current_user_chord = {};
  std::vector<arp_table_note> _current_arp_note_table = {};

  int flipped_table_pos() const;
  float current_mod_val(int mod_shape);
  void hard_reset(std::vector<note_event>& out);

  template <int ModMode>
  void process_audio(
    plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes);

public:

  arpeggiator_engine();

  int 
  current_arp_note_table_size() const
  { return _current_arp_note_table.size(); }

  void
  build_arp_note_table(
    std::array<arp_user_note, 128> const& user_chord,
    int type, int mode, int seed, bool jump);

  void process_audio(
    plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  void reset_audio(
    plugin_block const* block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override {};
};

std::unique_ptr<module_engine>
make_arpeggiator() 
{ return std::make_unique<arpeggiator_engine>(); }

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = max_active_table_size;
  return result;
}

static std::unique_ptr<graph_engine>
make_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param,
  param_topo_mapping const& mapping, std::vector<mod_out_custom_state> const& custom_outputs)
{
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, mapping.param_slot).step();
  int mode = state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, mapping.param_slot).step();
  int seed = state.get_plain_at(mapping.module_index, mapping.module_slot, param_seed, mapping.param_slot).step();
  bool jump = state.get_plain_at(mapping.module_index, mapping.module_slot, param_jump, mapping.param_slot).step() != 0;
  if (type == type_off) return graph_data(graph_data_type::off, {});

  std::vector<note_event> in_notes;
  std::vector<note_event> out_notes;

  // default to c major for plotting
  note_event note;
  note.frame = 0;
  note.id.id = 0;
  note.id.channel = 0;
  note.velocity = 1.0f;
  note.type = note_event_type::on;

  // TODO fix the table freq for plotting and ditch the lfo
  note.id.key = 60;
  in_notes.push_back(note);
  note.id.key = 64;
  in_notes.push_back(note);
  note.id.key = 79; // TODO just checking
  in_notes.push_back(note);

  std::array<arp_user_note, 128> user_chord = {};
  for (int i = 0; i < in_notes.size(); i++)
  {
    user_chord[in_notes[i].id.key].on = true;
    user_chord[in_notes[i].id.key].velocity = 1.0f;
  }

  arpeggiator_engine arp_engine;
  arp_engine.build_arp_note_table(user_chord, type, mode, seed, jump);
  int table_size = arp_engine.current_arp_note_table_size();
  engine->process_begin(&state, table_size, table_size, -1);
  auto const* block = engine->process(mapping.module_index, mapping.module_slot, custom_outputs, nullptr, [&](plugin_block& block) {
    arp_engine.reset_graph(&block, &in_notes, &out_notes, custom_outputs, nullptr);
    arp_engine.process_graph(block, &in_notes, &out_notes, custom_outputs, nullptr);
  });
  engine->process_end();
  auto const& rel_out_notes = block->state.own_cv[output_rel_note][0];
  jarray<float, 1> series(std::vector<float>(rel_out_notes.data().begin(), rel_out_notes.data().begin() + table_size));
  return graph_data(series, false, 1.0f, false, {});
}

module_topo
arpeggiator_topo(plugin_topo const* topo, int section, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{8A09B4CD-9768-4504-B9FE-5447B047854B}", true, "Arpeggiator", "Arp", "Arp", module_arpeggiator, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info_basic("{94A509ED-AB5B-43CF-B4F1-422815D99186}", "AbsPos", output_abs_pos, 1)),
      make_module_dsp_output(true, make_topo_info_basic("{ED2AF50B-64F4-4E21-9E66-95079A1E101B}", "RelPos", output_rel_pos, 1)),
      make_module_dsp_output(true, make_topo_info_basic("{AAA54A01-75AF-475B-B02D-F85295462335}", "AbsNote", output_abs_note, 1)),
      make_module_dsp_output(true, make_topo_info_basic("{C17077E6-FDA9-4DAE-9FAB-E6499A45F462}", "RelNote", output_rel_note, 1)) }),
    make_module_gui(section, pos, { { 1 }, { 24, 11, 28 } })));
  result.gui.tabbed_name = "ARP";
  result.info.description = "Arpeggiator";
  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_graph_engine;

  result.sections.emplace_back(make_param_section(section_table,
    make_topo_tag_basic("{6779AFA8-E0FE-482F-989B-6DE07263AEED}", "Table"),
    make_param_section_gui({ 0, 0 }, { { 1, 1 }, { 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{FF418A06-2017-4C23-BC65-19FAF226ABE8}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), "Off"),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = "TODO";
  auto& jump = result.params.emplace_back(make_param(
    make_topo_info("{41D984D0-2ECA-463C-B32E-0548562658AE}", true, "Table Jumping", "Jmp", "Jmp", param_jump, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_table, gui_edit_type::toggle, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  jump.info.description = "TODO";
  jump.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{BCE75C3A-85B8-4946-A06B-68F8F5F36785}", "Mode", param_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mode_items(), "Up"),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mode.info.description = "TODO";
  mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& flip = result.params.emplace_back(make_param(
    make_topo_info("{35217232-FFFF-4286-8C87-3E08D9817B8D}", true, "Table Flip Count", "Flip", "Flip", param_flip, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 9, 1, 0),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  flip.info.description = "TODO";
  flip.gui.bindings.visible.bind_params({ param_type, param_mode }, [](auto const& vs) { return !is_random(vs[1]); });
  flip.gui.bindings.enabled.bind_params({ param_type, param_mode }, [](auto const& vs) { return vs[0] != type_off && !is_random(vs[1]); });
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info_basic("{BCF494BD-5643-414C-863E-324F022770BE}", "Seed", param_seed, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_table, gui_edit_type::knob, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  seed.info.description = "TODO";
  seed.gui.bindings.visible.bind_params({ param_type, param_mode }, [](auto const& vs) { return is_random(vs[1]); });
  seed.gui.bindings.enabled.bind_params({ param_type, param_mode }, [](auto const& vs) { return vs[0] != type_off && is_random(vs[1]); });

  result.sections.emplace_back(make_param_section(section_notes,
    make_topo_tag_basic("{46584D75-5785-4475-A7E5-1EDEB3E925B1}", "Notes"),
    make_param_section_gui({ 0, 1 }, { { 1, 1 }, {
      gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  auto& notes = result.params.emplace_back(make_param(
    make_topo_info("{9E464846-650B-42DC-9E45-D8AFA2BADBDB}", true, "Note Count", "Notes", "Notes", param_notes, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 4, 1, 0),
    make_param_gui_single(section_notes, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  notes.info.description = "TODO";
  notes.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& dist = result.params.emplace_back(make_param(
    make_topo_info("{4EACE8F8-6B15-4336-9904-0375EE935CBD}", true, "Note Distance", "Dist", "Dist", param_dist, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 8, 1, 0),
    make_param_gui_single(section_notes, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist.info.description = "TODO";
  dist.gui.bindings.enabled.bind_params({ param_type, param_notes }, [](auto const& vs) { return vs[0] != type_off && vs[1] > 1; });

  auto& sample_section = result.sections.emplace_back(make_param_section(section_sample,
    make_topo_tag_basic("{63A54D7E-C4CE-4DFF-8E00-A9B8FAEC643E}", "Sample"),
    make_param_section_gui({ 0, 2 }, { { 1, 1 }, { gui_dimension::auto_size, 1, 1, 1, 1, 1, 1 } })));
  sample_section.gui.autofit_row = 1;
  auto& rate_hz = result.params.emplace_back(make_param(
    make_topo_info_basic("{EE305C60-8D37-492D-A2BE-5BD9C80DC59D}", "Rate", param_rate_hz, 1),
    make_param_dsp_block(param_automate::automate), make_domain_log(0.25, 20, 4, 4, 2, "Hz"),
    make_param_gui_single(section_sample, gui_edit_type::hslider, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_hz.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  rate_hz.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] == 0; });
  rate_hz.info.description = "TODO";
  auto& rate_tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{B1727889-9B55-4F93-8E0A-E2D4B791568B}", "Rate", param_rate_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_tempo.gui.submenu = make_timesig_submenu(rate_tempo.domain.timesigs);
  rate_tempo.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  rate_tempo.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] != 0; });
  rate_tempo.info.description = "TODO";  
  auto& rate_mod_rate_hz = result.params.emplace_back(make_param(
    make_topo_info_basic("{BCE11AB3-3BB2-43CD-AD5E-C0E62B73F6E0}", "Mod Rate", param_rate_mod_rate_hz, 1),
    make_param_dsp_block(param_automate::automate), make_domain_log(0.25, 20, 4, 4, 2, "Hz"),
    make_param_gui_single(section_sample, gui_edit_type::hslider, { 0, 1, 1, 3 },
      make_label_none())));
  rate_mod_rate_hz.gui.bindings.enabled.bind_params({ param_type, param_sync, param_rate_mod_mode }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0 && vs[2] != 0; });
  rate_mod_rate_hz.gui.bindings.visible.bind_params({ param_type, param_sync, param_rate_mod_mode }, [](auto const& vs) { return vs[1] == 0; });
  rate_mod_rate_hz.info.description = "TODO";
  auto& rate_mod_rate_tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{075B311C-51B0-46FB-994A-7C222F7BB60A}", "Mod Rate", param_rate_mod_rate_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 0, 1, 1, 3 },
      make_label_none())));
  rate_mod_rate_tempo.gui.submenu = make_timesig_submenu(rate_mod_rate_tempo.domain.timesigs);
  rate_mod_rate_tempo.gui.bindings.enabled.bind_params({ param_type, param_sync, param_rate_mod_mode }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0 && vs[2] != 0; });
  rate_mod_rate_tempo.gui.bindings.visible.bind_params({ param_type, param_sync, param_rate_mod_mode }, [](auto const& vs) { return vs[1] != 0; });
  rate_mod_rate_tempo.info.description = "TODO";
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info_basic("{8DE4D902-946C-41AA-BA1B-E0B645F8C87D}", "Snc", param_sync, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_sample, gui_edit_type::toggle, { 0, 4, 1, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  sync.info.description = "TODO";
  sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& rate_mod = result.params.emplace_back(make_param(
    make_topo_info_basic("{3545206C-7A5F-41A3-B418-1F270DF61505}", "Mod", param_rate_mod_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(wave_shape_type_items(wave_target::arp, true), "Saw"),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_mod.info.description = "TODO";
  rate_mod.gui.bindings.enabled.bind_params({ param_type, param_rate_mod_mode }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  auto& mod_mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{48F3CE67-54B0-4F1C-927B-DED1BD65E6D6}", "Mode", param_rate_mod_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mod_mode_items(), "Off"),
    make_param_gui_single(section_sample, gui_edit_type::list, { 1, 1, 1, 4 },
      make_label_none())));
  mod_mode.info.description = "TODO";
  mod_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& rate_mod_amt = result.params.emplace_back(make_param(
    make_topo_info_basic("{90A4DCE9-9EEA-4156-AC9F-DAD82ED33048}", "Amt", param_rate_mod_amt, 1),
    make_param_dsp_block(param_automate::automate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_sample, gui_edit_type::knob, { 1, 5, 1, 2 },
      make_label_none())));
  rate_mod_amt.info.description = "TODO";
  rate_mod_amt.gui.bindings.enabled.bind_params({ param_type, param_rate_mod_mode }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });

  return result;
}         

arpeggiator_engine::
arpeggiator_engine()
{
  // should be enough to accomodate anything, hopefully
  _current_arp_note_table.reserve(max_active_table_size);
}

void 
arpeggiator_engine::hard_reset(std::vector<note_event>& out)
{
  for (int i = 0; i < 128; i++)
  {
    note_event off;
    off.frame = 0;
    off.id.id = 0;
    off.id.key = i;
    off.id.channel = 0; // TODO need this?
    off.velocity = 0.0f;
    off.type = note_event_type::off;
    out.push_back(off);
  }
}

int
arpeggiator_engine::flipped_table_pos() const
{
  int n = _table_pos % (_prev_flip * 2);
  if (n < _prev_flip) return _table_pos % _current_arp_note_table.size();
  n -= _prev_flip;
  int base_pos = _table_pos - (_table_pos % _prev_flip);
  int result = (base_pos + _prev_flip - n - 1) % _current_arp_note_table.size();
  assert(0 <= result && result < _current_arp_note_table.size());
  return result;
}

inline float
arpeggiator_engine::current_mod_val(int mod_shape)
{
  switch (mod_shape)
  {
  case wave_shape_type_saw: return wave_shape_uni_saw(_mod_phase);
  case wave_shape_type_tri: return wave_shape_uni_tri(_mod_phase);
  case wave_shape_type_sin: return wave_shape_uni_sin(_mod_phase);
  case wave_shape_type_cos: return wave_shape_uni_cos(_mod_phase);
  case wave_shape_type_sqr_or_fold: return wave_shape_uni_sqr(_mod_phase);
  case wave_shape_type_sin_sin: return wave_shape_uni_sin_sin(_mod_phase);
  case wave_shape_type_sin_cos: return wave_shape_uni_sin_cos(_mod_phase);
  case wave_shape_type_cos_sin: return wave_shape_uni_cos_sin(_mod_phase);
  case wave_shape_type_cos_cos: return wave_shape_uni_cos_cos(_mod_phase);
  case wave_shape_type_sin_sin_sin: return wave_shape_uni_sin_sin_sin(_mod_phase);
  case wave_shape_type_sin_sin_cos: return wave_shape_uni_sin_sin_cos(_mod_phase);
  case wave_shape_type_sin_cos_sin: return wave_shape_uni_sin_cos_sin(_mod_phase);
  case wave_shape_type_sin_cos_cos: return wave_shape_uni_sin_cos_cos(_mod_phase);
  case wave_shape_type_cos_sin_sin: return wave_shape_uni_cos_sin_sin(_mod_phase);
  case wave_shape_type_cos_sin_cos: return wave_shape_uni_cos_sin_cos(_mod_phase);
  case wave_shape_type_cos_cos_sin: return wave_shape_uni_cos_cos_sin(_mod_phase);
  case wave_shape_type_cos_cos_cos: return wave_shape_uni_cos_cos_cos(_mod_phase);
  default: assert(false); return 0.0f;
  }
}

void
arpeggiator_engine::build_arp_note_table(
  std::array<arp_user_note, 128> const& user_chord, 
  int type, int mode, int seed, bool jump)
{
  _current_arp_note_table.clear();

  // STEP 1: build up the base chord table
  for (int i = 0; i < 128; i++)
    if (user_chord[i].on)
    {
      arp_table_note atn;
      atn.midi_key = i;
      atn.velocity = user_chord[i].velocity;
      _current_arp_note_table.push_back(atn);
    }

  if (_current_arp_note_table.size() == 0)
    return;

  // STEP 2: take the +/- oct into account
  bool add_m3 = false;
  bool add_m2 = false;
  bool add_m1 = false;
  bool add_p1 = false;
  bool add_p2 = false;
  bool add_p3 = false;
  switch (type)
  {
  case type_straight: break;
  case type_p1_oct: add_p1 = true; break;
  case type_p2_oct: add_p1 = true; add_p2 = true; break;
  case type_p3_oct: add_p1 = true; add_p2 = true; add_p3 = true; break;
  case type_m1p1_oct: add_m1 = true; add_p1 = true; break;
  case type_m2p2_oct: add_m1 = true; add_p1 = true; add_m2 = true; add_p2 = true; break;
  case type_m3p3_oct: add_m1 = true; add_p1 = true; add_m2 = true; add_p2 = true; add_m3 = true; add_p3 = true; break;
  default: assert(false); break;
  }

  int base_note_count = _current_arp_note_table.size();
  if (add_m3)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 36, _current_arp_note_table[i].velocity });
  if (add_m2)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 24, _current_arp_note_table[i].velocity });
  if (add_m1)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 12, _current_arp_note_table[i].velocity });
  if (add_p1)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 12, _current_arp_note_table[i].velocity });
  if (add_p2)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 24, _current_arp_note_table[i].velocity });
  if (add_p3)
    for (int i = 0; i < base_note_count && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 36, _current_arp_note_table[i].velocity });

  // make sure all is in check
  // we cannot go out of bounds in pitch
  // pitch 2 freq translator allows it, but we won't be able to kill MIDI -3 or MIDI 134 etc
  for (int i = 0; i < _current_arp_note_table.size(); i++)
    _current_arp_note_table[i].midi_key = std::clamp(_current_arp_note_table[i].midi_key, 0, 127);

  // and sort the thing
  auto comp = [](auto const& l, auto const& r) { return l.midi_key < r.midi_key; };
  std::sort(_current_arp_note_table.begin(), _current_arp_note_table.end(), comp);

  // need for cv outputs
  for (int i = 0; i < _current_arp_note_table.size(); i++)
  {
    _note_abs_mapping[_current_arp_note_table[i].midi_key] = i;
    _note_low_key = std::min(_note_low_key, _current_arp_note_table[i].midi_key);
    _note_high_key = std::max(_note_high_key, _current_arp_note_table[i].midi_key);
  }

  // TODO handle seeding for graph
  // STEP 3: take the up-down into account
  int note_set_count = _current_arp_note_table.size();
  switch (mode)
  {
  case mode_rand:
  case mode_fixrand:
  case mode_rand_free:
  case mode_fixrand_free:
    if (mode == mode_fixrand || mode == mode_fixrand_free)
      _random.seed(seed);
    std::shuffle(_current_arp_note_table.begin(), _current_arp_note_table.end(), _random);
    break;
  case mode_up:
    break; // already sorted
  case mode_down:
    std::reverse(_current_arp_note_table.begin(), _current_arp_note_table.end());
    break;
  case mode_up_down1: // excluding first/last (ceg->cege)
    for (int i = note_set_count - 2; i >= 1 && _current_arp_note_table.size() < max_active_table_size; i--)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
    break;
  case mode_up_down2:
    for (int i = note_set_count - 1; i >= 0 && _current_arp_note_table.size() < max_active_table_size; i--) // including first/last (ceg->ceggec)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
    break;
  case mode_down_up1:
    std::reverse(_current_arp_note_table.begin(), _current_arp_note_table.end());
    for (int i = note_set_count - 2; i >= 1 && _current_arp_note_table.size() < max_active_table_size; i--) // excluding first/last (ceg->gece)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
    break;
  case mode_down_up2:
    std::reverse(_current_arp_note_table.begin(), _current_arp_note_table.end());
    for (int i = note_set_count - 1; i >= 0 && _current_arp_note_table.size() < max_active_table_size; i--) // including first/last (ceg->gecceg)
      _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
    break;
  default:
    assert(false);
    break;
  }

  // STEP 4 jump: cega -> cecgca
  if (jump != 0)
  {
    note_set_count = _current_arp_note_table.size();
    for (int i = 0; i < note_set_count - 2 && _current_arp_note_table.size() < max_active_table_size; i++)
      _current_arp_note_table.insert(_current_arp_note_table.begin() + 2 + 2 * i, _current_arp_note_table[0]);
  }
}

void
arpeggiator_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  switch (block.state.own_block_automation[param_rate_mod_mode][0].step())
  {
  case mod_mode_off: process_audio<mod_mode_off>(block, in_notes, out_notes); break;
  case mod_mode_linear: process_audio<mod_mode_linear>(block, in_notes, out_notes); break;
  case mod_mode_exp: process_audio<mod_mode_exp>(block, in_notes, out_notes); break;
  default: assert(false); break;
  }
}

template <int ModMode> void 
arpeggiator_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  // plugin_base clears on each round
  assert(out_notes->size() == 0);

  auto& own_cv = block.state.own_cv;
  auto const& block_auto = block.state.own_block_automation;
  int flip = block_auto[param_flip][0].step();
  int type = block_auto[param_type][0].step();
  int mode = block_auto[param_mode][0].step();
  int seed = block_auto[param_seed][0].step();
  int dist = block_auto[param_dist][0].step();
  int jump = block_auto[param_jump][0].step();
  int sync = block_auto[param_sync][0].step();
  int notes = block_auto[param_notes][0].step();
  int mod_type = block_auto[param_rate_mod_type][0].step();

  // hard reset to make sure none of these are sticky
  if (type != _prev_type || mode != _prev_mode || flip != _prev_flip || notes != _prev_notes 
    || seed != _prev_seed || jump != _prev_jump || dist != _prev_dist || sync != _prev_sync)
  {
    _prev_type = type;
    _prev_mode = mode;
    _prev_flip = flip;
    _prev_seed = seed;
    _prev_dist = dist;
    _prev_jump = jump;
    _prev_sync = sync;
    _prev_notes = notes;
    hard_reset(*out_notes);
  }

  if (type == type_off)
  {
    out_notes->insert(out_notes->end(), in_notes->begin(), in_notes->end());
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      own_cv[output_abs_pos][0][f] = 0.0f;
      own_cv[output_rel_pos][0][f] = 0.0f;
      own_cv[output_rel_note][0][f] = 0.0f;
      own_cv[output_abs_note][0][f] = 0.0f;
    }
    return;
  }
  
  // this assumes notes are ordered by stream pos
  bool table_changed = false;
  for (int i = 0; i < in_notes->size(); i++)
  {
    table_changed = true;
    if ((*in_notes)[i].type == note_event_type::on)
    {
      _current_user_chord[(*in_notes)[i].id.key].on = true;
      _current_user_chord[(*in_notes)[i].id.key].velocity = (*in_notes)[i].velocity;
    }
    else
    {
      _current_user_chord[(*in_notes)[i].id.key].on = false;
      _current_user_chord[(*in_notes)[i].id.key].velocity = 0.0f;
    }
  }

  if (table_changed)
  {
    // STEP 0: off all notes
    // just active notes is not enough, 
    // user may have been playing with voice/osc note/oct in the meantime
    hard_reset(*out_notes);

    // reset to before start, will get picked up
    _table_pos = -1;
    _note_high_key = 0;
    _note_low_key = 128;
    _note_remaining = 0;
    _cv_out_abs_pos = 0.0f;
    _cv_out_rel_pos = 0.0f;
    _cv_out_abs_note = 0.0f;
    _cv_out_rel_note = 0.0f;
    build_arp_note_table(_current_user_chord, type, mode, seed, jump);
  }

  if (_current_arp_note_table.size() == 0)
    return;

  // how often to output new notes
  float rate_hz_base;
  if (sync == 0)
    rate_hz_base = block_auto[param_rate_hz][0].real();
  else
    rate_hz_base = timesig_to_freq(block.host.bpm, get_timesig_param_value(block, module_arpeggiator, param_rate_tempo));

  float mod_rate_hz;
  if (sync == 0)
    mod_rate_hz = block_auto[param_rate_mod_rate_hz][0].real();
  else
    mod_rate_hz = timesig_to_freq(block.host.bpm, get_timesig_param_value(block, module_arpeggiator, param_rate_mod_rate_tempo));

  float rate_mod_amt_base = block.state.own_block_automation[param_rate_mod_amt][0].real();
  assert(-1 <= rate_mod_amt_base && rate_mod_amt_base <= 1);

  // keep looping through the table, 
  // taking the flip parameter into account
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    int flipped_pos;
    if (_note_remaining == 0)
    {
      if (_table_pos != -1)
      {
        flipped_pos = is_random(mode)? _table_pos: flipped_table_pos();
        for (int i = 0; i < notes; i++)
        {
          note_event end_old = {};
          end_old.frame = f;
          end_old.velocity = 0.0f;
          end_old.type = note_event_type::off;
          end_old.id.id = 0;
          end_old.id.channel = 0; 
          end_old.id.key = _current_arp_note_table[(flipped_pos + i * dist) % _current_arp_note_table.size()].midi_key;
          out_notes->push_back(end_old);
        }
      }

      // apply modulation to arp rate
      float rate_hz = rate_hz_base;
      int rate_frames = block.sample_rate / rate_hz;
      int rate_frames_base = block.sample_rate / rate_hz;

      if constexpr (ModMode != mod_mode_off)
      {
        // for graphing just go with equal length
        // the mod indicator will show the speed
        if (!block.graph)
        {
          float mod_val = current_mod_val(mod_type);
          check_unipolar(mod_val);

          float actual_mod_val = mod_val * rate_mod_amt_base;
          if constexpr (ModMode == mod_mode_linear)
            actual_mod_val *= mod_range_linear;
          else
            actual_mod_val *= mod_range_exp;

          // if synced, make sure we are multiple of the tempo
          if (sync)
            actual_mod_val = std::round(actual_mod_val);

          // speed up - add to the frequency
          if (actual_mod_val >= 0.0f)
          {
            if constexpr (ModMode == mod_mode_linear)
              rate_hz = rate_hz_base + actual_mod_val * rate_hz_base;
            else
              rate_hz = rate_hz_base + (std::pow(2, actual_mod_val) - 1) * rate_hz_base;
            rate_frames = block.sample_rate / rate_hz;
          }
          else
          {
            // slow down - add to the frame count
            if constexpr (ModMode == mod_mode_linear)
              rate_frames = rate_frames_base - actual_mod_val * rate_frames_base;
            else
              rate_frames = rate_frames_base + (std::pow(2, -actual_mod_val) - 1) * rate_frames_base;
          }
        }
      }

      _table_pos++;
      _note_remaining = block.graph? 1: rate_frames;

      // TODO this better works WRT to note-off events ?
      if (mode == mode_rand_free || mode == mode_fixrand_free)
        if (_table_pos % _current_arp_note_table.size() == 0)
          std::shuffle(_current_arp_note_table.begin(), _current_arp_note_table.end(), _random);

      flipped_pos = is_random(mode) ? _table_pos : flipped_table_pos();

      // update current values for cv state
      _cv_out_abs_note = 0.0f;
      _cv_out_rel_note = 0.0f;
      _cv_out_rel_pos = flipped_pos / (float)(_current_arp_note_table.size() - 1);
      _cv_out_abs_pos = (_table_pos % _current_arp_note_table.size()) / (float)(_current_arp_note_table.size() - 1);
      int note_abs_range = _note_high_key - _note_low_key;
      if (note_abs_range > 0)
      {
        _cv_out_rel_note = (_current_arp_note_table[flipped_pos].midi_key - _note_low_key) / (float)note_abs_range;
        _cv_out_abs_note = _note_abs_mapping[_current_arp_note_table[flipped_pos].midi_key] / (float)(_current_arp_note_table.size() - 1);
      }

      for (int i = 0; i < notes; i++)
      {
        note_event start_new = {};
        start_new.frame = f;
        start_new.type = note_event_type::on;
        start_new.velocity = _current_arp_note_table[(flipped_pos + i * dist) % _current_arp_note_table.size()].velocity;
        start_new.id.id = 0;
        start_new.id.channel = 0;
        start_new.id.key = _current_arp_note_table[(flipped_pos + i * dist) % _current_arp_note_table.size()].midi_key;
        out_notes->push_back(start_new);
      }
    }
    
    --_note_remaining;
    own_cv[output_abs_pos][0][f] = _cv_out_abs_pos;
    own_cv[output_rel_pos][0][f] = _cv_out_rel_pos;
    own_cv[output_abs_note][0][f] = _cv_out_abs_note;
    own_cv[output_rel_note][0][f] = _cv_out_rel_note;

    if constexpr (ModMode != mod_mode_off)
    {
      increment_and_wrap_phase(_mod_phase, mod_rate_hz, block.sample_rate);
    }
  }
}

}
