#pragma once

#include <plugin_base/engine.hpp>
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace plugin_base::vst3 {

class component final:
public Steinberg::Vst::AudioEffect {
  plugin_engine _engine;
  using int32 = Steinberg::int32;
  using SpeakerArrangement = Steinberg::Vst::SpeakerArrangement;
public:
  component(std::unique_ptr<plugin_topo>&& topo, Steinberg::FUID const& controller_id);
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolic_size) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 input_count, SpeakerArrangement* outputs, int32 output_count) override;
};

}
