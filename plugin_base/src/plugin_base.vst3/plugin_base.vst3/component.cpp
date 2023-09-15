#include <plugin_base/io.hpp>
#include <plugin_base.vst3/component.hpp>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

component::
component(std::unique_ptr<plugin_topo>&& topo, FUID const& controller_id) :
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
  if(_engine.desc().plugin->type == plugin_type::fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (_engine.desc().plugin->type != plugin_type::fx && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if((_engine.desc().plugin->type == plugin_type::fx) && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API
component::getState(IBStream* state)
{
  io_store_file(*_engine.desc().plugin, _engine.state(), "C:\\temp\\plug.json");
  return kResultFalse;
}

tresult PLUGIN_API
component::setState(IBStream* state)
{
  io_load(*_engine.desc().plugin, {}, _engine.state());
  return kResultFalse;
}

tresult PLUGIN_API
component::process(ProcessData& data)
{
  host_block& block = _engine.prepare();
  block.common->frame_count = data.numSamples;
  block.common->bpm = data.processContext ? data.processContext->tempo : 0;
  block.common->audio_in = _engine.desc().plugin->type == plugin_type::fx? data.inputs[0].channelBuffers32: nullptr;
  block.common->stream_time = data.processContext ? data.processContext->projectTimeSamples : 0;
  block.common->audio_out = data.outputs[0].channelBuffers32;

  Event vst_event;
  if (data.inputEvents)
    for (int i = 0; i < data.inputEvents->getEventCount(); i++)
      if (data.inputEvents->getEvent(i, vst_event))
      {
        if (vst_event.type == Event::kNoteOnEvent) 
        {
          note_event note;
          note.type = note_event::type::on;
          note.frame = vst_event.sampleOffset;
          note.velocity = vst_event.noteOn.velocity;
          note.id.id = vst_event.noteOn.noteId;
          note.id.key = vst_event.noteOn.pitch;
          note.id.channel = vst_event.noteOn.channel;
          block.common->notes.push_back(note);
        }
        else if (vst_event.type == Event::kNoteOffEvent)
        {
          note_event note;
          note.type = note_event::type::off;
          note.frame = vst_event.sampleOffset;
          note.velocity = vst_event.noteOff.velocity;
          note.id.id = vst_event.noteOff.noteId;
          note.id.key = vst_event.noteOff.pitch;
          note.id.channel = vst_event.noteOff.channel;
          block.common->notes.push_back(note);
        }
      }

  ParamValue value;
  IParamValueQueue* queue;
  int frame_index = 0;
  if(data.inputParameterChanges)
    for(int i = 0; i < data.inputParameterChanges->getParameterCount(); i++)
      if ((queue = data.inputParameterChanges->getParameterData(i)) != nullptr)
      {
        int param_global_index = _engine.desc().id_to_index.at(queue->getParameterId());
        auto const& mapping = _engine.desc().mappings[param_global_index];
        auto rate = _engine.desc().param_at(mapping).param->rate;
        if (rate == param_rate::block && queue->getPoint(0, frame_index, value) == kResultTrue)
        {
          host_block_event event;
          event.normalized = normalized_value(value);
          event.param_global_index = param_global_index;
          block.block_events.push_back(event);
        }
        else if (rate == param_rate::accurate)
          for(int p = 0; p < queue->getPointCount(); p++)
            if (queue->getPoint(p, frame_index, value) == kResultTrue)
            {
              host_accurate_event event;
              event.frame_index = frame_index;
              event.normalized = normalized_value(value);
              event.param_global_index = param_global_index;
              block.accurate_events.push_back(event);
            }
      }
  
  _engine.process();

  int unused_index = 0;
  for (int e = 0; e < block.output_events.size(); e++)
  {
    auto const& event = block.output_events[e];
    int param_tag = _engine.desc().index_to_id[event.param_global_index];
    queue = data.outputParameterChanges->addParameterData(param_tag, unused_index);
    queue->addPoint(0, event.normalized.value(), unused_index);
  }

  return kResultOk;
}

}