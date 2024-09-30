#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>
#include <array>

using namespace plugin_base;
 
namespace firefly_synth {

enum { section_main };
enum { 
  param_type, param_mode,
  ///* TODO param_cv_source, */ param_oct_down, param_oct_up//, param_sync, 
  //param_rate_time, param_length_time, param_rate_tempo, param_length_tempo,
  /* TODO param_rate_offset_source, amt, length_offset_source, amt */
};

enum { mode_up, mode_down, mode_up_down, mode_down_up };
enum { type_off, type_straight, type_p1_oct, type_p2_oct, type_m1p1_oct, type_m2p2_oct };

struct arp_note_state
{
  bool on = {};
  float velocity = {};
};

struct arp_active_note
{
  int midi_key;
  float velocity;
};

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{70109417-1525-48A6-AE1D-7AB0E5765310}", "Off");
  result.emplace_back("{85A091D9-1283-4E67-961E-48C57BC68EB7}", "Straight");
  result.emplace_back("{20CDFF90-9D2D-4AFD-8138-1BCB61370F23}", "+1 Oct");
  result.emplace_back("{4935E002-3745-4097-887F-C8ED52213658}", "+2 Oct");
  result.emplace_back("{018FFDF0-AA2F-40BB-B449-34C8E93BCEB2}", "+/-1 Oct");
  result.emplace_back("{E802C511-30E0-4B9B-A548-173D4C807AFF}", "+/-2 Oct");
  return result;
}

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{25F4EF71-60E4-4F60-B613-8549C1BA074B}", "Up");
  result.emplace_back("{1772EDDE-6EC2-4F72-AC98-5B521AFB0EF1}", "Down");
  result.emplace_back("{1ECA59EC-B4B5-4EE9-A1E8-0169E7F32BCC}", "UpDown");
  result.emplace_back("{B48727A2-E886-43D4-906D-D87F5E7EE3CD}", "DownUp");
  return result;
}

class arpeggiator_engine :
public arp_engine_base
{
  // todo stuff with velo
  int _table_pos = 0;
  int _note_remaining = 0;
  std::int64_t _start_time = 0;
  std::vector<arp_active_note> _active_notes = {};
  std::array<arp_note_state, 128> _current_chord = {};

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
arpeggiator_topo(int section, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info_basic("{8A09B4CD-9768-4504-B9FE-5447B047854B}", "ARP / SEQ", module_arpeggiator, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
    make_module_gui(section, pos, { 1, 1 })));
  result.info.description = "Arpeggiator / Sequencer.";

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{6779AFA8-E0FE-482F-989B-6DE07263AEED}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, gui_dimension::auto_size } })));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{FF418A06-2017-4C23-BC65-19FAF226ABE8}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), "Off"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = "TODO";
  auto& mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{BCE75C3A-85B8-4946-A06B-68F8F5F36785}", "Mode", param_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mode_items(), "Up"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mode.info.description = "TODO";
  return result;
}         

arpeggiator_engine::
arpeggiator_engine()
{ _active_notes.reserve(128); /* guess */ }

void 
arpeggiator_engine::process_notes(
  plugin_block const& block,
  std::vector<note_event> const& in,
  std::vector<note_event>& out)
{
  // plugin_base clears on each round
  assert(out.size() == 0);

  // todo if switching block params, output a bunch of note offs
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  int mode = block_auto[param_mode][0].step();
  if (type == type_off)
  {
    out.insert(out.end(), in.begin(), in.end());
    return;
  }
  
  // todo do the reset-on-event-thing
  // todo do the lookahead window thing / allow some time to accumulate the table
  // buildup thing is for hw keyboards as i doubt anyone can hit keys sample-accurate
  // TODO allow to cut vs off
  
  // this assumes notes are ordered by stream pos
  int table_changed_frame = 0; // TODO something with this
  bool table_changed = false; // make this TODO a bit more lenient
  for (int i = 0; i < in.size(); i++)
  {
    table_changed = true;
    table_changed_frame = in[i].frame;
    if (in[i].type == note_event_type::on)
    {
      _current_chord[in[i].id.key].on = true;
      _current_chord[in[i].id.key].velocity = in[i].velocity;
    }
    else
    {
      _current_chord[in[i].id.key].on = false;
      _current_chord[in[i].id.key].velocity = 0.0f;
    }
  }

  if (table_changed)
  {
    _table_pos = 0;
    _note_remaining = 0;
    _active_notes.clear();

    // STEP 1: build up the base chord table
    for (int i = 0; i < 128; i++)
      if(_current_chord[i].on)
      {
        arp_active_note aan;
        aan.midi_key = i;
        aan.velocity = _current_chord[i].velocity;
        _active_notes.push_back(aan);
      }

    if (_active_notes.size() == 0)
      return;

    // STEP 2: take the +/- oct into account
    int base_note_count = _active_notes.size();
    switch (type)
    {
    case type_straight: 
      break;
    case type_p1_oct:
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 12, _active_notes[i].velocity });
      break;
    case type_p2_oct:
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 12, _active_notes[i].velocity });
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 24, _active_notes[i].velocity });
      break;
    case type_m1p1_oct:
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key - 12, _active_notes[i].velocity });
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 12, _active_notes[i].velocity });
      break;
    case type_m2p2_oct:
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key - 12, _active_notes[i].velocity });
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key - 24, _active_notes[i].velocity });
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 12, _active_notes[i].velocity });
      for (int i = 0; i < base_note_count; i++)
        _active_notes.push_back({ _active_notes[i].midi_key + 24, _active_notes[i].velocity });
      break;
    default:
      assert(false);
      break;
    }

    // and sort the thing
    auto comp = [](auto const& l, auto const& r) { return l.midi_key < r.midi_key; };
    std::sort(_active_notes.begin(), _active_notes.end(), comp);

    // STEP 3: take the up-down into account
    int note_set_count = _active_notes.size();
    switch (mode)
    {
    case mode_up:
      break; // already sorted
    case mode_down:
      std::reverse(_active_notes.begin(), _active_notes.end());
      break;
    case mode_up_down:
      for (int i = note_set_count - 2; i >= 1; i--)
        _active_notes.push_back({ _active_notes[i].midi_key, _active_notes[i].velocity });
      break;
    case mode_down_up:
      std::reverse(_active_notes.begin(), _active_notes.end());
      for (int i = note_set_count - 2; i >= 1; i--)
        _active_notes.push_back({ _active_notes[i].midi_key, _active_notes[i].velocity });
      break;
    default:
      assert(false);
      break;
    }
  }

  if (_active_notes.size() == 0)
    return;

  // TODO what would actually happen when "play chord", release, wait, "play chord"
  // TODO stuff with actual start pos of the table as based on note event frame
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_note_remaining == 0) // TODO make sure this is always 1+ frames
    {
      note_event end_old = {};
      end_old.id.id = 0;
      end_old.id.channel = 0; // TODO maybe ?
      end_old.id.key = _active_notes[_table_pos].midi_key;
      end_old.frame = f;
      end_old.velocity = 0.0f;
      end_old.type = note_event_type::off; // TODO
      out.push_back(end_old);

      _note_remaining = 12000; // TODO
      _table_pos = (_table_pos + 1) % _active_notes.size();

      note_event start_new = {};
      start_new.id.id = 0;
      start_new.id.channel = 0;
      start_new.id.key = _active_notes[_table_pos].midi_key;
      start_new.frame = f;
      start_new.type = note_event_type::on;
      start_new.velocity = _active_notes[_table_pos].velocity;
      out.push_back(start_new);
    }
    
    --_note_remaining;
  }
}

}
