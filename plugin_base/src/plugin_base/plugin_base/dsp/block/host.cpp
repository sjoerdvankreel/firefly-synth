#include <plugin_base/dsp/block/host.hpp>
#include <cmath>

namespace plugin_base {

void
host_block::prepare()
{
  audio_out = nullptr;
  events.midi.clear();
  events.notes.clear();
  events.block.clear();
  events.output_params.clear();
  events.mod_indicator_states.clear();
  events.accurate_automation.clear();
  events.accurate_modulation.clear();
  events.accurate_automation_and_modulation.clear();

  frame_count = 0;
  shared.bpm = 0;
  shared.audio_in = nullptr;
}
  
void 
host_events::deactivate()
{
  midi = {};
  notes = {};
  block = {};
  output_params = {};
  mod_indicator_states = {};
  accurate_automation = {};
  accurate_modulation = {};
  accurate_automation_and_modulation = {};
}

void 
host_events::activate(bool graph, int module_count, int param_count, int midi_count, int polyphony, int max_frame_count)
{ 
  // reserve this much but allocate on the audio thread if necessary
  // still seems better than dropping events
  // for graphing, no need to pre-allocate anything
  int fill_guess = graph? 0: (int)std::ceil(max_frame_count / 32.0f);
  int block_events_guess = graph? 0: param_count;
  int note_limit_guess = polyphony * fill_guess;
  int midi_events_guess = midi_count * fill_guess;
  int accurate_events_guess = param_count * fill_guess;
  int mod_indicator_states_guess = module_count * polyphony;

  midi.reserve(midi_events_guess);
  notes.reserve(note_limit_guess);
  block.reserve(block_events_guess);
  output_params.reserve(block_events_guess);

  // see also plugin_engine::ctor
  mod_indicator_states.reserve(mod_indicator_states_guess);

  accurate_automation.reserve(accurate_events_guess);
  accurate_modulation.reserve(accurate_events_guess);
  accurate_automation_and_modulation.reserve(accurate_events_guess);
}

}