#pragma once

#include <plugin_base/engine.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block/host.hpp>

#include <public.sdk/source/vst/vstaudioeffect.h>

namespace plugin_base::vst3 {

class inf_component final:
public Steinberg::Vst::AudioEffect {
  plugin_engine _engine;

public:
  INF_DECLARE_MOVE_ONLY(inf_component);
  inf_component(std::unique_ptr<plugin_topo>&& topo, Steinberg::FUID const& controller_id);

  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolic_size) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 input_count,
    Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 output_count) override;
};

}
