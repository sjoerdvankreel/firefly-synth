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
  param_type, /* TODO param_cv_source, */ param_oct_down, param_oct_up//, param_sync, 
  //param_rate_time, param_length_time, param_rate_tempo, param_length_tempo,
  /* TODO param_rate_offset_source, amt, length_offset_source, amt */
};

enum { type_off, type_up, type_down, type_up_down, type_down_up, type_cv_source };

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
  result.emplace_back("{85A091D9-1283-4E67-961E-48C57BC68EB7}", "Up");
  result.emplace_back("{20CDFF90-9D2D-4AFD-8138-1BCB61370F23}", "Down");
  result.emplace_back("{4935E002-3745-4097-887F-C8ED52213658}", "UpDown");
  result.emplace_back("{018FFDF0-AA2F-40BB-B449-34C8E93BCEB2}", "DownUp");
  result.emplace_back("{E802C511-30E0-4B9B-A548-173D4C807AFF}", "CV Source");
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
    make_param_section_gui({ 0, 0 }, { 1, 1 })));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{FF418A06-2017-4C23-BC65-19FAF226ABE8}", "Mode", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), "Off"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = "TODO";
  auto& oct_down = result.params.emplace_back(make_param(
    make_topo_info_basic("{FF418A06-2017-4C23-BC65-19FAF226ABE8}", "Mode", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), "Off"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.info.description = "TODO";
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
  if (block_auto[param_type][0].step() == type_off)
  {
    out.insert(out.end(), in.begin(), in.end());
    return;
  }
  
  // todo do the reset-on-event-thing
  // todo do the lookahead window thing / allow some time to accumulate the table
  // buildup thing is for hw keyboards as i doubt anyone can hit keys sample-accurate
  // TODO allow to cut vs off
  
  // this assumes notes are ordered by stream pos
  // TODO ok this gets me an "active" table -- what now?
  int table_changed_frame = 0;
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
    for (int i = 0; i < 128; i++)
      if(_current_chord[i].on)
      {
        arp_active_note aan;
        aan.midi_key = i;
        aan.velocity = _current_chord[i].velocity;
        _active_notes.push_back(aan);
      }
  }

  if (_active_notes.empty())
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

      _note_remaining = 4800; // TODO
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
