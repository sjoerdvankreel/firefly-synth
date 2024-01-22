#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cassert>

namespace plugin_base {

template <int MaxLanes>
class oversampler {
  
  int const _max_frame_count;
  juce::dsp::Oversampling<float> _2x;
  juce::dsp::Oversampling<float> _4x;
  juce::dsp::Oversampling<float> _8x;

  template <class NonLinear>
  juce::dsp::AudioBlock<float> process_off(
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, NonLinear non_linear);

  template <int Factor, class NonLinear>
  juce::dsp::AudioBlock<float> process(
    juce::dsp::Oversampling<float>& oversampling, 
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, NonLinear non_linear);

  static juce::dsp::Oversampling<float>::FilterType filter_type(bool iir);

public:
  PB_PREVENT_ACCIDENTAL_COPY(oversampler);
  oversampler(int max_frame_count, bool iir, bool max_quality, bool integer_latency);
  
  template <class NonLinear>
  juce::dsp::AudioBlock<float> process(int stages,
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, NonLinear non_linear);
};

template <int MaxLanes>
juce::dsp::Oversampling<float>::FilterType
oversampler<MaxLanes>::filter_type(bool iir)
{
  if (iir) return juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;
  return juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple;
}

template <int MaxLanes>
oversampler<MaxLanes>::
oversampler(int max_frame_count, bool iir, bool max_quality, bool integer_latency) :
  _max_frame_count(max_frame_count),
  _2x(MaxLanes * 2, 1, filter_type(iir), max_quality, integer_latency),
  _4x(MaxLanes * 2, 2, filter_type(iir), max_quality, integer_latency),
  _8x(MaxLanes * 2, 3, filter_type(iir), max_quality, integer_latency)
{
  _2x.initProcessing(max_frame_count);
  _4x.initProcessing(max_frame_count);
  _8x.initProcessing(max_frame_count);
}

template <int MaxLanes>
template <class NonLinear> 
juce::dsp::AudioBlock<float>
oversampler<MaxLanes>::process_off(
  std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, NonLinear non_linear)
{
  float* not_upsampled[MaxLanes * 2] = { nullptr };
  for (int l = 0; l < active_lanes; l++)
  {
    not_upsampled[l * 2 + 0] = (*inout[l])[0].data().data();
    not_upsampled[l * 2 + 1] = (*inout[l])[1].data().data();
  }
  for (int f = start_frame; f < end_frame; f++)
    non_linear(not_upsampled, f);
  float* data[MaxLanes * 2] = { nullptr };
  for (int l = 0; l < active_lanes; l++)
  {
    data[l * 2 + 0] = (*inout[l])[0].data().data();
    data[l * 2 + 1] = (*inout[l])[1].data().data();
  }
  int block_size = end_frame - start_frame;
  return juce::dsp::AudioBlock<float>(data, active_lanes * 2, start_frame, block_size);
}

template <int MaxLanes>
template <int Factor, class NonLinear> 
juce::dsp::AudioBlock<float>
oversampler<MaxLanes>::process(
  juce::dsp::Oversampling<float>& oversampling, 
  std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, NonLinear non_linear)
{
  static_assert(Factor == 1 || Factor == 2 || Factor == 4 || Factor == 8);
  using juce::dsp::AudioBlock;
  int block_size = end_frame - start_frame;
  float* data[MaxLanes * 2] = { nullptr };
  for(int l = 0; l < active_lanes; l++)
  {
    data[l * 2 + 0] = (*inout[l])[0].data().data();
    data[l * 2 + 1] = (*inout[l])[1].data().data();
  }
  AudioBlock<float> inout_block(data, active_lanes * 2, start_frame, block_size);
  auto up_block = oversampling.processSamplesUp(inout_block);
  float* upsampled[MaxLanes * 2] = { nullptr };
  for (int l = 0; l < active_lanes; l++)
  {
    upsampled[l * 2 + 0] = up_block.getChannelPointer(l * 2 + 0);
    upsampled[l * 2 + 1] = up_block.getChannelPointer(l * 2 + 1);
  }
  for (int f = 0; f < block_size * Factor; f++)
    non_linear(upsampled, f);
  oversampling.processSamplesDown(inout_block);
  return up_block;
}

template <int MaxLanes>
template <class NonLinear> 
juce::dsp::AudioBlock<float>
oversampler<MaxLanes>::process(
  int stages, std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, NonLinear non_linear)
{
  using juce::dsp::AudioBlock;
  assert(0 <= stages && stages <= 3);
  switch (stages)
  {
  case 0: return process_off(inout, active_lanes, start_frame, end_frame, non_linear); break;
  case 1: return process<2>(_2x, inout, active_lanes, start_frame, end_frame, non_linear); break;
  case 2: return process<4>(_4x, inout, active_lanes, start_frame, end_frame, non_linear); break;
  case 3: return process<8>(_8x, inout, active_lanes, start_frame, end_frame, non_linear); break;
  default: assert(false); return {};
  }
}

}
