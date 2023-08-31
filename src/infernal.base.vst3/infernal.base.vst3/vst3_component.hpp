#pragma once
#include <infernal.base/plugin_topo.hpp>
#include <infernal.base/plugin_engine.hpp>
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace infernal::base::vst3 {

class vst3_component final:
public Steinberg::Vst::AudioEffect {
  plugin_engine _engine;
  using int32 = Steinberg::int32;
  using SpeakerArrangement = Steinberg::Vst::SpeakerArrangement;
public:
  vst3_component(plugin_topo&& topo, Steinberg::FUID const& controller_id);
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolic_size) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 input_count, SpeakerArrangement* outputs, int32 output_count) override;
};

}
