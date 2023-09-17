#include <plugin_base/io.hpp>
#include <plugin_base.vst3/inf_component.hpp>

#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

inf_component::
inf_component(std::unique_ptr<plugin_topo>&& topo, FUID const& controller_id) :
_engine(std::move(topo))
{
  setControllerClass(controller_id);
  processContextRequirements.needTempo();
  processContextRequirements.needFrameRate();
  processContextRequirements.needContinousTimeSamples();
}

tresult PLUGIN_API
inf_component::canProcessSampleSize(int32 symbolic_size)
{
  if (symbolic_size == kSample32) return kResultTrue;
  return kResultFalse;
}

tresult PLUGIN_API
inf_component::setupProcessing(ProcessSetup& setup)
{
  _engine.activate(setup.sampleRate, setup.maxSamplesPerBlock);
  return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API
inf_component::getState(IBStream* state)
{
  plugin_io io(&_engine.desc());
  std::vector<char> data(io.save(_engine.state()));
  return state->write(data.data(), data.size());
}

tresult PLUGIN_API
inf_component::setState(IBStream* state)
{
  char byte;
  int read = 1;
  std::vector<char> data;
  while (state->read(&byte, 1, &read) == kResultTrue && read == 1)
    data.push_back(byte);
  plugin_io io(&_engine.desc());
  if (io.load(data, _engine.state()).ok()) return kResultOk;
  return kResultFalse;
}

tresult PLUGIN_API
inf_component::initialize(FUnknown* context)
{
  if(AudioEffect::initialize(context) != kResultTrue) return kResultFalse;
  addEventInput(STR16("Event In"));
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  if(_engine.desc().plugin->type == plugin_type::fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
inf_component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (_engine.desc().plugin->type != plugin_type::fx && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if((_engine.desc().plugin->type == plugin_type::fx) && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API
inf_component::process(ProcessData& data)
{
  host_block& block = _engine.prepare();
  block.audio_out = data.outputs[0].channelBuffers32;
  block.common->frame_count = data.numSamples;
  block.common->bpm = data.processContext ? data.processContext->tempo : 0;
  block.common->stream_time = data.processContext ? data.processContext->projectTimeSamples : 0;
  block.common->audio_in = _engine.desc().plugin->type == plugin_type::fx? data.inputs[0].channelBuffers32: nullptr;

  Event vst_event;
  if (data.inputEvents)
    for (int i = 0; i < data.inputEvents->getEventCount(); i++)
      if (data.inputEvents->getEvent(i, vst_event))
      {
        if (vst_event.type == Event::kNoteOnEvent) 
        {
          note_event note;
          note.type = note_event::type_t::on;
          note.frame = vst_event.sampleOffset;
          note.id.id = vst_event.noteOn.noteId;
          note.id.key = vst_event.noteOn.pitch;
          note.id.channel = vst_event.noteOn.channel;
          note.velocity = vst_event.noteOn.velocity;
          block.common->notes.push_back(note);
        }
        else if (vst_event.type == Event::kNoteOffEvent)
        {
          note_event note;
          note.type = note_event::type_t::off;
          note.frame = vst_event.sampleOffset;
          note.id.id = vst_event.noteOff.noteId;
          note.id.key = vst_event.noteOff.pitch;
          note.id.channel = vst_event.noteOff.channel;
          note.velocity = vst_event.noteOff.velocity;
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
        int param_index = _engine.desc().param_tag_to_index.at(queue->getParameterId());
        auto const& mapping = _engine.desc().mappings[param_index];
        auto rate = _engine.desc().param_at(mapping).param->rate;
        if (rate == param_rate::block && queue->getPoint(0, frame_index, value) == kResultTrue)
        {
          block_event event;
          event.param = param_index;
          event.normalized = normalized_value(value);
          block.events.block.push_back(event);
        }
        else if (rate == param_rate::accurate)
          for(int p = 0; p < queue->getPointCount(); p++)
            if (queue->getPoint(p, frame_index, value) == kResultTrue)
            {
              accurate_event event;
              event.frame = frame_index;
              event.param = param_index;
              event.normalized = normalized_value(value);
              block.events.accurate.push_back(event);
            }
      }
  
  _engine.process();
  int unused_index = 0;
  for (int e = 0; e < block.events.out.size(); e++)
  {
    auto const& event = block.events.out[e];
    int tag = _engine.desc().param_index_to_tag[event.param];
    queue = data.outputParameterChanges->addParameterData(tag, unused_index);
    queue->addPoint(0, event.normalized.value(), unused_index);
  }
  return kResultOk;
}

}