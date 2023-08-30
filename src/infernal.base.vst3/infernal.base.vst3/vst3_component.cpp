#include <infernal.base.vst3/vst3_component.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

vst3_component::
vst3_component(plugin_topo&& topo, FUID const& controller_id) :
_engine(std::move(topo))
{
  setControllerClass(controller_id);
  processContextRequirements.needTempo();
  processContextRequirements.needContinousTimeSamples();
}

tresult PLUGIN_API
vst3_component::canProcessSampleSize(int32 symbolic_size)
{
  if (symbolic_size == kSample32) return kResultTrue;
  return kResultFalse;
}

tresult PLUGIN_API
vst3_component::setupProcessing(ProcessSetup& setup)
{
  _engine.activate(setup.sampleRate, setup.maxSamplesPerBlock);
  return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API
vst3_component::initialize(FUnknown* context)
{
  if(AudioEffect::initialize(context) != kResultTrue) return kResultFalse;
  addEventInput(STR16("Event In"));
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  if(_engine.topo().static_topo.is_fx) addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  return kResultTrue;
}

tresult PLUGIN_API
vst3_component::setBusArrangements(
  SpeakerArrangement* inputs, int32 input_count,
  SpeakerArrangement* outputs, int32 output_count)
{
  if (!_engine.topo().static_topo.is_fx && input_count != 0) return kResultFalse;
  if (output_count != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  if(_engine.topo().static_topo.is_fx && (input_count != 1 || inputs[0] != SpeakerArr::kStereo))  return kResultFalse;
  return AudioEffect::setBusArrangements(inputs, input_count, outputs, output_count);
}

tresult PLUGIN_API
vst3_component::process(ProcessData& data)
{
  return kResultOk;
}

}