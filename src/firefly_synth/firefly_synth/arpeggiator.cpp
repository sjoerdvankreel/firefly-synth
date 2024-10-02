#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/dsp.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <array>
#include <random>
#include <algorithm>

using namespace plugin_base;
 
namespace firefly_synth {

enum { 
  section_table, section_sample };

enum { 
  param_type, param_bounce, param_notes,
  param_mode, param_flip, param_seed, param_dist,
  param_sync, param_rate_hz, param_rate_tempo, 
  param_rate_mod, param_rate_mod_amt };

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
make_mod_sources(plugin_topo const* topo)
{
  // bit of manual juggling here since this stuff
  // does not belong in any of the existing matrices
  std::vector<list_item> result;
  result.emplace_back("{E7BBFFE3-5105-453A-A40E-22B4FDEB1456}", "Off");

  auto const& aux_outputs = topo->modules[module_global_in].dsp.outputs;
  for(int i = 0; i < aux_outputs.size(); i++)
    for (int j = 0; j < aux_outputs[i].info.slot_count; j++)
    result.emplace_back("{AB65DAA5-6437-483D-8B71-37000B3451B4}-" + std::to_string(i) + "-" + std::to_string(j),
      topo->modules[module_global_in].info.tag.display_name + " " + 
      aux_outputs[i].info.tag.menu_display_name + (aux_outputs[i].info.slot_count == 1 ? "" : " " + std::to_string(j + 1)));

  auto const& lfo_mod = topo->modules[module_glfo];
  assert(lfo_mod.dsp.outputs.size() == 1);
  assert(lfo_mod.dsp.outputs[0].info.slot_count == 1);
  for (int i = 0; i < lfo_mod.info.slot_count; i++)
    result.emplace_back("{93C5C173-3E67-41FE-B30D-8B3DCC5D8966}-" + std::to_string(i) + "-0-0",
      lfo_mod.info.tag.menu_display_name + " " + std::to_string(i + 1));

  return result;
}

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{70109417-1525-48A6-AE1D-7AB0E5765310}", "Off");
  result.emplace_back("{85A091D9-1283-4E67-961E-48C57BC68EB7}", "Plain");
  result.emplace_back("{20CDFF90-9D2D-4AFD-8138-1BCB61370F23}", "+1 Oct");
  result.emplace_back("{4935E002-3745-4097-887F-C8ED52213658}", "+2 Oct");
  result.emplace_back("{DCC943F4-5447-413F-B741-A83F2B84C259}", "+3 Oct");
  result.emplace_back("{018FFDF0-AA2F-40BB-B449-34C8E93BCEB2}", "+/-1 Oct");
  result.emplace_back("{E802C511-30E0-4B9B-A548-173D4C807AFF}", "+/-2 Oct");
  result.emplace_back("{7EA59B15-D0AE-4B6F-8A04-0AA4630F5E39}", "+/-3 Oct");
  return result;
}

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{25F4EF71-60E4-4F60-B613-8549C1BA074B}", "Up");
  result.emplace_back("{1772EDDE-6EC2-4F72-AC98-5B521AFB0EF1}", "Down");
  result.emplace_back("{1ECA59EC-B4B5-4EE9-A1E8-0169E7F32BCC}", "Up Down 1");
  result.emplace_back("{EB5FC7ED-DFA3-4DD5-A5F3-444469FDFBCF}", "Up Down 2");
  result.emplace_back("{B48727A2-E886-43D4-906D-D87F5E7EE3CD}", "Down Up 1");
  result.emplace_back("{86488834-DB23-4467-8EB6-4C4261989233}", "Down Up 2");
  result.emplace_back("{C9095A2C-3F11-4C4B-A428-6DC948DFDB2C}", "Rnd");
  result.emplace_back("{05A6B86C-1DCC-41F0-BE03-03B7D932FE5B}", "Fix Rnd");
  result.emplace_back("{DC858447-2FB6-4081-BE94-4C3F9FB835EB}", "Rnd Free");
  result.emplace_back("{39619505-DD48-4B91-92F4-5CDDBECC8872}", "Fix Rnd Free");
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
  int _prev_notes = -1;
  int _prev_bounce = -1;

  int _table_pos = -1;
  int _note_remaining = 0;
  std::default_random_engine _random = {};

  std::array<arp_user_note, 128> _current_user_chord = {};
  std::vector<arp_table_note> _current_arp_note_table = {};

  int flipped_table_pos() const;
  void hard_reset(std::vector<note_event>& out);

public:

  arpeggiator_engine();

  void process_notes(
    plugin_block const& block,
    std::vector<note_event> const& in,
    std::vector<note_event>& out) override;
};

std::unique_ptr<arp_engine_base>
make_arpeggiator() { return std::make_unique<arpeggiator_engine>(); }

module_topo
arpeggiator_topo(plugin_base::plugin_topo const* topo, int section, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info_basic("{8A09B4CD-9768-4504-B9FE-5447B047854B}", "ARP / SEQ", module_arpeggiator, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
    make_module_gui(section, pos, { { 1 }, { 2, 1 } })));
  result.info.description = "Arpeggiator / Sequencer.";

  result.sections.emplace_back(make_param_section(section_table,
    make_topo_tag_basic("{6779AFA8-E0FE-482F-989B-6DE07263AEED}", "Table"),
    make_param_section_gui({ 0, 0 }, { { 1, 1 }, { 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, gui_dimension::auto_size_all,
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{FF418A06-2017-4C23-BC65-19FAF226ABE8}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), "Off"),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = "TODO";
  auto& bounce = result.params.emplace_back(make_param(
    make_topo_info_basic("{41D984D0-2ECA-463C-B32E-0548562658AE}", "Bounce", param_bounce, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_table, gui_edit_type::toggle, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  bounce.info.description = "TODO";
  bounce.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& notes = result.params.emplace_back(make_param(
    make_topo_info_basic("{9E464846-650B-42DC-9E45-D8AFA2BADBDB}", "Notes", param_notes, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 4, 1, 0),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  notes.info.description = "TODO";
  notes.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{BCE75C3A-85B8-4946-A06B-68F8F5F36785}", "Mode", param_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mode_items(), "Up"),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mode.info.description = "TODO";
  mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& flip = result.params.emplace_back(make_param(
    make_topo_info_basic("{35217232-FFFF-4286-8C87-3E08D9817B8D}", "Flip", param_flip, 1),
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
  auto& dist = result.params.emplace_back(make_param(
    make_topo_info_basic("{4EACE8F8-6B15-4336-9904-0375EE935CBD}", "Dist", param_dist, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 8, 1, 0),
    make_param_gui_single(section_table, gui_edit_type::autofit_list, { 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist.info.description = "TODO";
  dist.gui.bindings.enabled.bind_params({ param_type, param_notes }, [](auto const& vs) { return vs[0] != type_off && vs[1] > 1; });

  result.sections.emplace_back(make_param_section(section_sample,
    make_topo_tag_basic("{63A54D7E-C4CE-4DFF-8E00-A9B8FAEC643E}", "Sample"),
    make_param_section_gui({ 0, 1 }, { { 1, 1 }, { 1, 1 } })));
  auto& sync = result.params.emplace_back(make_param(
    make_topo_info_basic("{8DE4D902-946C-41AA-BA1B-E0B645F8C87D}", "Sync", param_sync, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_sample, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  sync.info.description = "TODO";
  sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& rate_hz = result.params.emplace_back(make_param(
    make_topo_info_basic("{EE305C60-8D37-492D-A2BE-5BD9C80DC59D}", "Rate", param_rate_hz, 1),
    make_param_dsp_block(param_automate::automate), make_domain_log(0.25, 20, 4, 4, 2, "Hz"),
    make_param_gui_single(section_sample, gui_edit_type::knob, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_hz.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] == 0; });
  rate_hz.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] == 0; });
  rate_hz.info.description = "TODO";
  auto& rate_tempo = result.params.emplace_back(make_param(
    make_topo_info_basic("{B1727889-9B55-4F93-8E0A-E2D4B791568B}", "Rate", param_rate_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(false, { 16, 1 }, { 1, 4 }), // TODO tune this
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_tempo.gui.submenu = make_timesig_submenu(rate_tempo.domain.timesigs);
  rate_tempo.gui.bindings.enabled.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[0] != type_off && vs[1] != 0; });
  rate_tempo.gui.bindings.visible.bind_params({ param_type, param_sync }, [](auto const& vs) { return vs[1] != 0; });
  rate_tempo.info.description = "TODO";  
  auto& rate_mod = result.params.emplace_back(make_param(
    make_topo_info_basic("{3545206C-7A5F-41A3-B418-1F270DF61505}", "Mod", param_rate_mod, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(make_mod_sources(topo), "Off"),
    make_param_gui_single(section_sample, gui_edit_type::autofit_list, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_mod.info.description = "TODO";
  rate_mod.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& rate_mod_amt = result.params.emplace_back(make_param(
    make_topo_info_basic("{90A4DCE9-9EEA-4156-AC9F-DAD82ED33048}", "Amt", param_rate_mod_amt, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_sample, gui_edit_type::toggle, { 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  rate_mod_amt.info.description = "TODO";
  rate_mod_amt.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

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

void 
arpeggiator_engine::process_notes(
  plugin_block const& block,
  std::vector<note_event> const& in,
  std::vector<note_event>& out)
{
  // plugin_base clears on each round
  assert(out.size() == 0);

  // e.g. if table is abcdefgh
  // flip = 1 sequence is abcdefgh
  // flip = 2 sequence is abdcefhg
  // flip = 3 sequence is abcfedgh
  auto const& block_auto = block.state.own_block_automation;
  int flip = block_auto[param_flip][0].step();
  int type = block_auto[param_type][0].step();
  int mode = block_auto[param_mode][0].step();
  int seed = block_auto[param_seed][0].step();
  int dist = block_auto[param_dist][0].step();
  int notes = block_auto[param_notes][0].step();
  int bounce = block_auto[param_bounce][0].step();

  // hard reset to make sure none of these are sticky
  if (type != _prev_type || mode != _prev_mode || flip != _prev_flip || 
    notes != _prev_notes || seed != _prev_seed || bounce != _prev_bounce || dist != _prev_dist)
  {
    _prev_type = type;
    _prev_mode = mode;
    _prev_flip = flip;
    _prev_seed = seed;
    _prev_dist = dist;
    _prev_notes = notes;
    _prev_bounce = bounce;
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

    // STEP 4 bounce: cega -> cecgca
    if (bounce != 0)
    {
      note_set_count = _current_arp_note_table.size();
      for (int i = 0; i < note_set_count - 2; i++)
        _current_arp_note_table.insert(_current_arp_note_table.begin() + 2 + 2 * i, _current_arp_note_table[0]);
    }
  }

  if (_current_arp_note_table.size() == 0)
    return;

  // how often to output new notes
  float rate_hz;
  if (block_auto[param_sync][0].step() == 0)
    rate_hz = block_auto[param_rate_hz][0].real();
  else
    rate_hz = timesig_to_freq(block.host.bpm, get_timesig_param_value(block, module_arpeggiator, param_rate_tempo));
  int rate_frames = block.sample_rate / rate_hz;

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

      _table_pos++;
      _note_remaining = rate_frames;
      flipped_pos = is_random(mode) ? _table_pos : flipped_table_pos();

      // TODO make the time continuous rate a modulatable target
      // TODO this better works WRT to note-off events ?
      if(mode == mode_rand_free || mode == mode_fixrand_free)
        if(_table_pos % _current_arp_note_table.size() == 0)
          std::shuffle(_current_arp_note_table.begin(), _current_arp_note_table.end(), _random);

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
  }
}

}
