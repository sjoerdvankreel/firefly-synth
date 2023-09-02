#include <infernal.base.vst3/component.hpp>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

component::
component(plugin_topo&& topo, FUID const& controller_id) :
_engine(std::move(topo))
{
  setControllerClass(controller_id);
  processContextRequirements.needTempo();
  processContextRequirements.needFrameRate();
  processContextRequirements.needContinousTimeSamples();
}

tresult PLUGIN_API
component::canProcessSampleSize(int32 symbolic_size)
{
  if (symbolic_size == kSample32) return kResultTrue;
  return kResultFalse;
}

tresult PLUGIN_API
component::setupProcessing(ProcessSetup& setup)
{
  _engine.activate(setup.sampleRate, setup.maxSamplesPerBlock);
  return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API
component::initialize(FUnknown* context)
{
  if(AudioEffect::initialize(context) != kResultTrue) return kResultFalse;
  addEventInput(STR16("Event In"));
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  if(_engine.topo().is_fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (!_engine.topo().is_fx && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if(_engine.topo().is_fx && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API
component::process(ProcessData& data)
{
  host_block& block = _engine.prepare();
  block.common->frame_count = data.numSamples;
  block.common->audio_output = data.outputs[0].channelBuffers32;
  block.common->bpm = data.processContext ? data.processContext->tempo : 0;
  block.common->audio_input = _engine.topo().is_fx? data.inputs[0].channelBuffers32: nullptr;
  block.common->stream_time = data.processContext ? data.processContext->projectTimeSamples : 0;

  Event vst_event;
  if (data.inputEvents)
    for (int i = 0; i < data.inputEvents->getEventCount() && i < _engine.topo().note_limit; i++)
      if (data.inputEvents->getEvent(i, vst_event))
        if (vst_event.type == Event::kNoteOnEvent) 
        {
          note_event note;
          note.type = note_event_type::on;
          note.frame_index = vst_event.sampleOffset;
          note.velocity = vst_event.noteOn.velocity;
          note.id.id = vst_event.noteOn.noteId;
          note.id.key = vst_event.noteOn.pitch;
          note.id.channel = vst_event.noteOn.channel;
          block.common->notes.push_back(note);
        }
        else if (vst_event.type == Event::kNoteOffEvent)
        {
          note_event note;
          note.type = note_event_type::off;
          note.frame_index = vst_event.sampleOffset;
          note.velocity = vst_event.noteOff.velocity;
          note.id.id = vst_event.noteOff.noteId;
          note.id.key = vst_event.noteOff.pitch;
          note.id.channel = vst_event.noteOff.channel;
          block.common->notes.push_back(note);
        }

  return kResultOk;
}

}