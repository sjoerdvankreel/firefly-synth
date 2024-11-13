#pragma once

#include <plugin_base/dsp/splice_engine.hpp>
#include <plugin_base/dsp/block/host.hpp>
#include <plugin_base/shared/utility.hpp>

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/utility/dataexchange.h>
#include <Client/libMTSClient.h>

#include <map>

namespace plugin_base::vst3 {

class pb_component final:
public Steinberg::Vst::AudioEffect {

  // MTS-ESP support
  MTSClient* _mts_client = {};
  
  // needs to be first, everyone else needs it
  std::unique_ptr<plugin_desc> _desc;
  plugin_splice_engine _splice_engine;
  std::map<int, int> _param_to_midi_id = {};

  // when we get passed a null buffer ptr
  std::vector<float> _scratch_in_l;
  std::vector<float> _scratch_in_r;
  std::vector<float> _scratch_out_l;
  std::vector<float> _scratch_out_r;

  // https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Data+Exchange.html
  std::unique_ptr<Steinberg::Vst::DataExchangeHandler> _exchange_handler = {};
  Steinberg::Vst::DataExchangeBlock _exchange_block = { nullptr, 0, Steinberg::Vst::InvalidDataExchangeBlockID };

public:
  PB_PREVENT_ACCIDENTAL_COPY(pb_component);
  ~pb_component() { MTS_DeregisterClient(_mts_client); }
  pb_component(plugin_topo const* topo, Steinberg::FUID const& controller_id);

  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API connect(Steinberg::Vst::IConnectionPoint* other) override;
  Steinberg::tresult PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint* other) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolic_size) override;

  Steinberg::tresult PLUGIN_API setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 input_count,
    Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 output_count) override;
};

}
