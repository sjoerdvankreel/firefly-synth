#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base.vst3/utility.hpp>
#include <plugin_base.vst3/pb_component.hpp>

#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

pb_component::
pb_component(plugin_topo const* topo, FUID const& controller_id) :
_desc(std::make_unique<plugin_desc>(topo, nullptr)),
_engine(_desc.get(), topo->audio_polyphony, nullptr, nullptr)
{
  setControllerClass(controller_id);
  processContextRequirements.needTempo();
  processContextRequirements.needFrameRate();
  processContextRequirements.needContinousTimeSamples();

  // each midi source can be attached to at most 1 module instance (topo+slot)!
  for(int m = 0; m < _desc->modules.size(); m++)
  {
    auto const& module = _desc->modules[m];
    for(int ms = 0; ms < module.midi_sources.size(); ms++)
    {
      auto const& source = module.midi_sources[ms];
      assert(_param_to_midi_id.find(source.info.id_hash) == _param_to_midi_id.end());
      _param_to_midi_id[source.info.id_hash] = source.source->id;
    }
  }
}

tresult PLUGIN_API
pb_component::getState(IBStream* state)
{
  std::vector<char> data(plugin_io_save_state(_engine.state()));
  return state->write(data.data(), data.size());
}

tresult PLUGIN_API
pb_component::setState(IBStream* state)
{
  if (!plugin_io_load_state(load_ibstream(state), _engine.state()).ok()) 
    return kResultFalse;
  _engine.mark_all_params_as_automated(true);
  return kResultOk;
}

tresult PLUGIN_API
pb_component::canProcessSampleSize(int32 symbolic_size)
{
  if (symbolic_size == kSample32) return kResultTrue;
  return kResultFalse;
}

tresult PLUGIN_API
pb_component::setupProcessing(ProcessSetup& setup)
{
  _engine.activate(setup.maxSamplesPerBlock);
  _engine.set_sample_rate(setup.sampleRate);
  _engine.activate_modules();
  return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API
pb_component::initialize(FUnknown* context)
{
  if(AudioEffect::initialize(context) != kResultTrue) return kResultFalse;
  addEventInput(STR16("Event In"));
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  if(_engine.state().desc().plugin->type == plugin_type::fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
pb_component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (_engine.state().desc().plugin->type != plugin_type::fx && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if((_engine.state().desc().plugin->type == plugin_type::fx) && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API
pb_component::process(ProcessData& data)
{
  host_block& block = _engine.prepare_block();
  block.frame_count = data.numSamples;
  block.audio_out = data.outputs[0].channelBuffers32;
  block.shared.bpm = data.processContext ? data.processContext->tempo : 0;
  block.shared.audio_in = _engine.state().desc().plugin->type == plugin_type::fx? data.inputs[0].channelBuffers32: nullptr;

  Event vst_event;
  if (data.inputEvents)
    for (int i = 0; i < data.inputEvents->getEventCount(); i++)
      if (data.inputEvents->getEvent(i, vst_event) == kResultOk)
      {
        if (vst_event.type == Event::kNoteOnEvent) 
        {
          note_event note;
          note.type = note_event_type::on;
          note.frame = vst_event.sampleOffset;
          note.id.id = vst_event.noteOn.noteId;
          note.id.key = vst_event.noteOn.pitch;
          note.id.channel = vst_event.noteOn.channel;
          note.velocity = vst_event.noteOn.velocity;
          block.events.notes.push_back(note);
        }
        else if (vst_event.type == Event::kNoteOffEvent)
        {
          note_event note;
          note.type = note_event_type::off;
          note.frame = vst_event.sampleOffset;
          note.id.id = vst_event.noteOff.noteId;
          note.id.key = vst_event.noteOff.pitch;
          note.id.channel = vst_event.noteOff.channel;
          note.velocity = vst_event.noteOff.velocity;
          block.events.notes.push_back(note);
        }
      }

  ParamValue value;
  IParamValueQueue* queue;
  int frame_index = 0;
  if(data.inputParameterChanges)
    for(int i = 0; i < data.inputParameterChanges->getParameterCount(); i++)
      if ((queue = data.inputParameterChanges->getParameterData(i)) != nullptr)
      {
        int param_id = queue->getParameterId();
        auto iter = _param_to_midi_id.find(param_id);
        if(iter != _param_to_midi_id.end())
        {
          // fake midi parameter - this is not part of the plugin's parameter topology
          // translate param events back to midi events for plugin_base
          for (int p = 0; p < queue->getPointCount(); p++)
            if (queue->getPoint(p, frame_index, value) == kResultTrue)
            {
              midi_event event;
              event.id = iter->second;
              event.frame = frame_index;
              event.normalized = normalized_value(value);
              block.events.midi.push_back(event);
            }
        }
        else
        {
          // regular parameter
          int param_index = _engine.state().desc().param_mappings.tag_to_index.at(queue->getParameterId());
          auto rate = _engine.state().desc().param_at_index(param_index).param->dsp.rate;
          if (rate != param_rate::accurate)
          {
            if(queue->getPoint(0, frame_index, value) == kResultTrue)
            {
              block_event event;
              event.param = param_index;
              event.normalized = normalized_value(value);
              block.events.block.push_back(event);
            }
          }
          else
          {
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
        }
      }
  
  _engine.process();
  int unused_index = 0;
  for (int e = 0; e < block.events.out.size(); e++)
  {
    auto const& event = block.events.out[e];
    int tag = _engine.state().desc().param_mappings.index_to_tag[event.param];
    queue = data.outputParameterChanges->addParameterData(tag, unused_index);
    queue->addPoint(0, event.normalized.value(), unused_index);
  }

  _engine.release_block();
  return kResultOk;
}

}