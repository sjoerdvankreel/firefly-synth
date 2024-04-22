#include <plugin_base/dsp/block/host.hpp>
#include <cmath>

namespace plugin_base {

void
host_block::prepare()
{
  audio_out = nullptr;
  events.out.clear();
  events.notes.clear();
  events.block.clear();
  events.midi.clear();
  events.accurate.clear();
  frame_count = 0;
  shared.bpm = 0;
  shared.audio_in = nullptr;
}
  
void 
host_events::deactivate()
{
  notes = {};
  out = {};
  block = {};
  midi = {};
  accurate = {};
}

void 
host_events::activate(bool graph, int param_count, int midi_count, int polyphony, int max_frame_count)
{ 
  // reserve this much but allocate on the audio thread if necessary
  // still seems better than dropping events
  // for graphing, no need to pre-allocate anything
  int fill_guess = graph? 0: (int)std::ceil(max_frame_count / 32.0f);
  int block_events_guess = graph? 0: param_count;
  int note_limit_guess = polyphony * fill_guess;
  int midi_events_guess = midi_count * fill_guess;
  int accurate_events_guess = param_count * fill_guess;

  notes.reserve(note_limit_guess);
  out.reserve(block_events_guess);
  block.reserve(block_events_guess);
  midi.reserve(midi_events_guess);
  accurate.reserve(accurate_events_guess);
}

}