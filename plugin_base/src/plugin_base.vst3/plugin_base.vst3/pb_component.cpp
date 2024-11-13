#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/shared/logger.hpp>
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
_mts_client(MTS_RegisterClient()),
_desc(std::make_unique<plugin_desc>(topo, nullptr)),
_splice_engine(_desc.get(), false, nullptr, nullptr)
{
  PB_LOG_FUNC_ENTRY_EXIT();

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
  PB_LOG_FUNC_ENTRY_EXIT();
  std::vector<char> data(plugin_io_save_instance_state(_splice_engine.state(), false));
  return state->write(data.data(), data.size());
}

tresult PLUGIN_API
pb_component::setState(IBStream* state)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  if (!plugin_io_load_instance_state(load_ibstream(state), _splice_engine.state(), false).ok())
    return kResultFalse;
  _splice_engine.automation_state_dirty();
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
  PB_LOG_FUNC_ENTRY_EXIT();
  _scratch_in_l.resize(setup.maxSamplesPerBlock);
  _scratch_in_r.resize(setup.maxSamplesPerBlock);
  _scratch_out_l.resize(setup.maxSamplesPerBlock);
  _scratch_out_r.resize(setup.maxSamplesPerBlock);
  _splice_engine.activate(setup.maxSamplesPerBlock);
  _splice_engine.set_sample_rate(setup.sampleRate);
  _splice_engine.activate_modules();
  return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API
pb_component::initialize(FUnknown* context)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  if(AudioEffect::initialize(context) != kResultTrue) return kResultFalse;
  addEventInput(STR16("Event In"));
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  if(_splice_engine.state().desc().plugin->type == plugin_type::fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
pb_component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (_splice_engine.state().desc().plugin->type == plugin_type::synth && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if((_splice_engine.state().desc().plugin->type == plugin_type::fx) && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API 
pb_component::setActive(TBool state)
{
  if (state)
    _exchange_handler->onActivate(processSetup);
  else
    _exchange_handler->onDeactivate();
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API 
pb_component::disconnect(IConnectionPoint* other)
{
  if (_exchange_handler)
  {
    _exchange_handler->onDisconnect(other);
    _exchange_handler.reset();
  }
  return AudioEffect::disconnect(other);
}

tresult PLUGIN_API
pb_component::connect(IConnectionPoint* other)
{
  tresult result = AudioEffect::connect(other);
  if (result != kResultTrue) return result;

  auto callback = [this](DataExchangeHandler::Config& config, ProcessSetup const& setup) {
    config.numBlocks = 1; // ui discards all but the last one
    config.alignment = 32;
    config.userContextID = 0;
    config.blockSize = sizeof(data_exchange_block_content);
    return true; };

  _exchange_handler = std::make_unique<DataExchangeHandler>(this, callback);
  _exchange_handler->onConnect(other, getHostContext());
  return kResultTrue;
}

tresult PLUGIN_API
pb_component::process(ProcessData& data)
{
  host_block& block = _splice_engine.prepare_block();
  block.mts_client = _mts_client;
  block.frame_count = data.numSamples;
  block.shared.bpm = data.processContext ? data.processContext->tempo : 0;
  block.shared.project_time = data.processContext ? data.processContext->projectTimeSamples: 0;

  if(data.outputs && data.outputs[0].channelBuffers32)
    block.audio_out = data.outputs[0].channelBuffers32;
  else
  {
    float* scratch_out[2] = { _scratch_out_l.data(), _scratch_out_r.data() };
    block.audio_out = scratch_out;
  }

  block.shared.audio_in = nullptr;
  if(_splice_engine.state().desc().plugin->type == plugin_type::fx)
    if(data.inputs && data.inputs[0].channelBuffers32)
      block.shared.audio_in = data.inputs[0].channelBuffers32;
    else
    {
      float* scratch_in[2] = { _scratch_in_l.data(), _scratch_in_r.data() };
      block.shared.audio_in = scratch_in;
    }

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
          // vst3 validator hands us bogus sometimes
          auto param_id_iter = _splice_engine.state().desc().param_mappings.tag_to_index.find(queue->getParameterId());
          if(param_id_iter != _splice_engine.state().desc().param_mappings.tag_to_index.end())
          {
            auto rate = _splice_engine.state().desc().param_at_index(param_id_iter->second).param->dsp.rate;
            if (rate != param_rate::accurate)
            {
              if(queue->getPoint(0, frame_index, value) == kResultTrue)
              {
                block_event event;
                event.param = param_id_iter->second;
                event.normalized = normalized_value(value);
                block.events.block.push_back(event);
              }
            }
            else
            {
              for(int p = 0; p < queue->getPointCount(); p++)
                if (queue->getPoint(p, frame_index, value) == kResultTrue)
                {
                  accurate_event automation_event;
                  automation_event.is_mod = false;
                  automation_event.frame = frame_index;
                  automation_event.param = param_id_iter->second;
                  automation_event.value_or_offset = (float)check_unipolar(value);
                  block.events.accurate_automation.push_back(automation_event);
                }
            }
          }
        }
      }
  
  _splice_engine.process();

  // regular output params
  int unused_index = 0;
  if(data.outputParameterChanges)
    for (int e = 0; e < block.events.output_params.size(); e++)
    {
      auto const& event = block.events.output_params[e];
      int tag = _splice_engine.state().desc().param_mappings.index_to_tag[event.param];
      queue = data.outputParameterChanges->addParameterData(tag, unused_index);
      queue->addPoint(0, event.normalized.value(), unused_index);
    }

  // module modulation outputs
  if (_exchange_block.blockID == Vst::InvalidDataExchangeBlockID)
  {
    _exchange_block = _exchange_handler->getCurrentOrNewBlock();
    if (_exchange_block.blockID != Vst::InvalidDataExchangeBlockID)
    {
      auto content = static_cast<data_exchange_block_content*>(_exchange_block.data);
      content->mod_output_count = std::min(data_exchange_block_content::max_mod_outputs, block.events.modulation_outputs.size());
      for (int i = 0; i < content->mod_output_count; i++)
        content->mod_outputs[i] = block.events.modulation_outputs[i];
    }
  }

  _splice_engine.release_block();

  return kResultOk;
}

}