#pragma once

#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/block/host.hpp>
#include <plugin_base/shared/utility.hpp>

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <map>

namespace plugin_base::vst3 {

class inf_component final:
public Steinberg::Vst::AudioEffect {
  plugin_engine _engine;

  std::map<int, int> _param_to_midi_id = {};

public:
  PB_PREVENT_ACCIDENTAL_COPY(inf_component);
  inf_component(plugin_desc const* desc, Steinberg::FUID const& controller_id);

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
