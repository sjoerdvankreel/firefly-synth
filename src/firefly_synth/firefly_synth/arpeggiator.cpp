#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/dsp.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>

#include <cmath>
#include <array>
#include <random>
#include <algorithm>

using namespace plugin_base;
 
namespace firefly_synth {

static float const min_mod_amt = -8.0f;
static float const max_mod_amt = 8.0f;

enum { 
  section_table, section_notes, section_sample };

enum {
  param_type, param_jump,
  param_mode, param_flip, param_seed,
  param_notes, param_dist,
  param_rate_hz, param_rate_tempo, param_rate_mod_rate_hz, param_rate_mod_rate_tempo, param_sync,
  param_rate_mod_type, param_rate_mod_amt, param_rate_mod_on };

enum { 
  mode_up, mode_down,
  mode_up_down1, mode_up_down2,
  mode_down_up1, mode_down_up2,
  mode_rand, mode_fixrand,
  mode_rand_free, mode_fixrand_free };

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
public arp_engine_base
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
  std::default_random_engine _random = {};

  std::array<arp_user_note, 128> _current_user_chord = {};
  std::vector<arp_table_note> _current_arp_note_table = {};

  int flipped_table_pos() const;
  float current_mod_val(int mod_shape);
  void hard_reset(std::vector<note_event>& out);

  template <bool IsModOn>
  void process_notes(
    plugin_block const& block,
    std::vector<note_event> const& in,
    std::vector<note_event>& out);

public:

  arpeggiator_engine();
  void process_notes(
    plugin_block const& block,
    std::vector<note_event> const& in,
    std::vector<note_event>& out) override;
};

std::unique_ptr<arp_engine_base>
make_arpeggiator() 
{ return std::make_unique<arpeggiator_engine>(); }

module_topo
arpeggiator_topo(plugin_topo const* topo, int section, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info_basic("{8A09B4CD-9768-4504-B9FE-5447B047854B}", "ARP / SEQ", module_arpeggiator, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
    make_module_gui(section, pos, { { 1 }, { 24, 11, 28 } })));
  result.info.description = "Arpeggiator / Sequencer.";

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
    make_param_section_gui({ 0, 2 }, { { 1, 1 }, { gui_dimension::auto_size, 1, 1 } })));
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
    make_param_gui_single(section_sample, gui_edit_type::hslider, { 0, 1 },
      make_label_none())));
  rate_mod_rate_hz.gui.bindings.enabled.bind_params({ param_type, param_sync, param_rate_mod_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0 && vs[2] != 0; });
  rate_mod_rate_hz.gui.bindings.visible.bind_params({ param_type, param_sync, param_rate_mod_on }, [](auto const& vs) { return vs[1] == 0; });
  rate_mod_rate_hz.info.description = "TODO";
  auto& rate_mod_rate_tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{075B311C-51B0-46FB-994A-7C222F7BB60A}", "Mod Rate", param_rate_mod_rate_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 0, 1 },
      make_label_none())));
  rate_mod_rate_tempo.gui.submenu = make_timesig_submenu(rate_mod_rate_tempo.domain.timesigs);
  rate_mod_rate_tempo.gui.bindings.enabled.bind_params({ param_type, param_sync, param_rate_mod_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0 && vs[2] != 0; });
  rate_mod_rate_tempo.gui.bindings.visible.bind_params({ param_type, param_sync, param_rate_mod_on }, [](auto const& vs) { return vs[1] != 0; });
  rate_mod_rate_tempo.info.description = "TODO";
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info_basic("{8DE4D902-946C-41AA-BA1B-E0B645F8C87D}", "Snc", param_sync, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_sample, gui_edit_type::toggle, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  sync.info.description = "TODO";
  sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& rate_mod = result.params.emplace_back(make_param(
    make_topo_info_basic("{3545206C-7A5F-41A3-B418-1F270DF61505}", "Mod", param_rate_mod_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(wave_shape_type_items(wave_target::arp, true), "Saw"),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_mod.info.description = "TODO";
  rate_mod.gui.bindings.enabled.bind_params({ param_type, param_rate_mod_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  auto& rate_mod_amt = result.params.emplace_back(make_param(
    make_topo_info_basic("{90A4DCE9-9EEA-4156-AC9F-DAD82ED33048}", "Amt", param_rate_mod_amt, 1),
    make_param_dsp_block(param_automate::automate), make_domain_percentage(min_mod_amt, max_mod_amt, 0, 0, true),
    make_param_gui_single(section_sample, gui_edit_type::knob, { 1, 1 },
      make_label_none())));
  rate_mod_amt.info.description = "TODO";
  rate_mod_amt.gui.bindings.enabled.bind_params({ param_type, param_rate_mod_on }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; }); 
  auto& mod_on = result.params.emplace_back(make_param(
    make_topo_info_basic("{48F3CE67-54B0-4F1C-927B-DED1BD65E6D6}", "On", param_rate_mod_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_sample, gui_edit_type::toggle, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mod_on.info.description = "TODO";
  mod_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  return result;
}         

arpeggiator_engine::
arpeggiator_engine()
{ _current_arp_note_table.reserve(128); /* guess */ }

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
arpeggiator_engine::process_notes(
  plugin_block const& block,
  std::vector<note_event> const& in,
  std::vector<note_event>& out)
{
  if (block.state.own_block_automation[param_rate_mod_on][0].step() == 0)
    process_notes<false>(block, in, out);
  else
    process_notes<true>(block, in, out);
}

template <bool IsModOn> void 
arpeggiator_engine::process_notes(
  plugin_block const& block,
  std::vector<note_event> const& in,
  std::vector<note_event>& out)
{
  // plugin_base clears on each round
  assert(out.size() == 0);

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
    hard_reset(out);
  }

  if (type == type_off)
  {
    out.insert(out.end(), in.begin(), in.end());
    return;
  }
  
  // this assumes notes are ordered by stream pos
  bool table_changed = false;
  for (int i = 0; i < in.size(); i++)
  {
    table_changed = true;
    if (in[i].type == note_event_type::on)
    {
      _current_user_chord[in[i].id.key].on = true;
      _current_user_chord[in[i].id.key].velocity = in[i].velocity;
    }
    else
    {
      _current_user_chord[in[i].id.key].on = false;
      _current_user_chord[in[i].id.key].velocity = 0.0f;
    }
  }

  if (table_changed)
  {
    // STEP 0: off all notes
    // just active notes is not enough, 
    // user may have been playing with voice/osc note/oct in the meantime
    hard_reset(out);

    // reset to before start, will get picked up
    _table_pos = -1;
    _note_remaining = 0;
    _current_arp_note_table.clear();

    // STEP 1: build up the base chord table
    for (int i = 0; i < 128; i++)
      if(_current_user_chord[i].on)
      {
        arp_table_note atn;
        atn.midi_key = i;
        atn.velocity = _current_user_chord[i].velocity;
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
    if(add_m3)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 36, _current_arp_note_table[i].velocity });
    if (add_m2)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 24, _current_arp_note_table[i].velocity });
    if (add_m1)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key - 12, _current_arp_note_table[i].velocity });
    if (add_p1)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 12, _current_arp_note_table[i].velocity });
    if (add_p2)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 24, _current_arp_note_table[i].velocity });
    if (add_p3)
      for (int i = 0; i < base_note_count; i++)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key + 36, _current_arp_note_table[i].velocity });

    // make sure all is in check
    // we cannot go out of bounds in pitch
    // pitch 2 freq translator allows it, but we won't be able to kill MIDI -3 or MIDI 134 etc
    for (int i = 0; i < _current_arp_note_table.size(); i++)
      _current_arp_note_table[i].midi_key = std::clamp(_current_arp_note_table[i].midi_key, 0, 127);

    // and sort the thing
    auto comp = [](auto const& l, auto const& r) { return l.midi_key < r.midi_key; };
    std::sort(_current_arp_note_table.begin(), _current_arp_note_table.end(), comp);

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
      for (int i = note_set_count - 2; i >= 1; i--)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
      break;
    case mode_up_down2:
      for (int i = note_set_count - 1; i >= 0; i--) // including first/last (ceg->ceggec)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
      break;
    case mode_down_up1:
      std::reverse(_current_arp_note_table.begin(), _current_arp_note_table.end());
      for (int i = note_set_count - 2; i >= 1; i--) // excluding first/last (ceg->gece)
        _current_arp_note_table.push_back({ _current_arp_note_table[i].midi_key, _current_arp_note_table[i].velocity });
      break;
    case mode_down_up2:
      std::reverse(_current_arp_note_table.begin(), _current_arp_note_table.end());
      for (int i = note_set_count - 1; i >= 0; i--) // including first/last (ceg->gecceg)
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
      for (int i = 0; i < note_set_count - 2; i++)
        _current_arp_note_table.insert(_current_arp_note_table.begin() + 2 + 2 * i, _current_arp_note_table[0]);
    }
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

  float rate_mod_amt = block.state.own_block_automation[param_rate_mod_amt][0].real();
  assert(min_mod_amt <= rate_mod_amt && rate_mod_amt <= max_mod_amt);

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
          out.push_back(end_old);
        }
      }

      // apply modulation to arp rate
      float rate_hz = rate_hz_base;
      int rate_frames = block.sample_rate / rate_hz;
      int rate_frames_base = block.sample_rate / rate_hz;

      if constexpr (IsModOn)
      {
        float mod_val = current_mod_val(mod_type);
        check_unipolar(mod_val);        
        float actual_mod_val = mod_val * rate_mod_amt;

        // if synced, make sure we are multiple of the tempo
        if (sync) 
          actual_mod_val = std::trunc(actual_mod_val);

        // speed up - add to the frequency
        if (actual_mod_val >= 0.0f)
        {
          rate_hz = rate_hz_base + actual_mod_val * rate_hz_base;
          rate_frames = block.sample_rate / rate_hz;
        }
        else
        {
          // slow down - add to the frame count
          rate_frames = rate_frames_base - actual_mod_val * rate_frames_base;
        }        
      }

      _table_pos++;
      _note_remaining = rate_frames;

      // TODO this better works WRT to note-off events ?
      if (mode == mode_rand_free || mode == mode_fixrand_free)
        if (_table_pos % _current_arp_note_table.size() == 0)
          std::shuffle(_current_arp_note_table.begin(), _current_arp_note_table.end(), _random);

      flipped_pos = is_random(mode) ? _table_pos : flipped_table_pos();

      for (int i = 0; i < notes; i++)
      {
        note_event start_new = {};
        start_new.frame = f;
        start_new.type = note_event_type::on;
        start_new.velocity = _current_arp_note_table[(flipped_pos + i * dist) % _current_arp_note_table.size()].velocity;
        start_new.id.id = 0;
        start_new.id.channel = 0;
        start_new.id.key = _current_arp_note_table[(flipped_pos + i * dist) % _current_arp_note_table.size()].midi_key;
        out.push_back(start_new);
      }
    }
    
    --_note_remaining;

    if constexpr (IsModOn)
    {
      increment_and_wrap_phase(_mod_phase, mod_rate_hz, block.sample_rate);
    }
  }
}

}
