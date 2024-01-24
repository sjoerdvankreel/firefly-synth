#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cassert>

namespace plugin_base {

inline int constexpr max_oversampler_stages = 3;

template <int MaxLanes>
class oversampler {
  
  int const _max_frame_count;

  // just put something here so we dont need to special case 1x oversampling
  jarray<float, 2> _1x;
  juce::dsp::Oversampling<float> _2x;
  juce::dsp::Oversampling<float> _4x;
  juce::dsp::Oversampling<float> _8x;

  std::array<float*, MaxLanes * 2> _1x_lanes_channels_ptrs = {};
  std::array<float*, MaxLanes * 2> _2x_lanes_channels_ptrs = {};
  std::array<float*, MaxLanes * 2> _4x_lanes_channels_ptrs = {};
  std::array<float*, MaxLanes * 2> _8x_lanes_channels_ptrs = {};

  template <class NonLinear>
  void process_off(
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear);

  template <int Factor, class NonLinear>
  void process(
    juce::dsp::Oversampling<float>& oversampling, 
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear);

  static juce::dsp::Oversampling<float>::FilterType filter_type(bool iir);

public:
  PB_PREVENT_ACCIDENTAL_COPY(oversampler);
  oversampler(int max_frame_count);
  
  float** get_upsampled_lanes_channels_ptrs(int factor);

  // set upsample to false if you don't need the input data but just the oversampled buffers
  template <class NonLinear>
  void process(int stages, 
    std::array<jarray<float, 2>*, MaxLanes> const& inout,
    int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear);
};

// Note: use the IIR filter rather than the FIR filter to minimize delay.
// Without delay compensation the delay is actually noticable in the 
// osc oversampling particularly when using oscillator FM. 
// And I don't want to write the delay compensation code.
template <int MaxLanes>
oversampler<MaxLanes>::
oversampler(int max_frame_count) :
  _max_frame_count(max_frame_count),
  _1x(MaxLanes * 2, jarray<float, 1>(max_frame_count, 0.0f)),
  _2x(MaxLanes * 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false),
  _4x(MaxLanes * 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false),
  _8x(MaxLanes * 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false)
{
  _2x.initProcessing(max_frame_count);
  _4x.initProcessing(max_frame_count);
  _8x.initProcessing(max_frame_count);

  // feed the oversamplers some silence to get a hold of the channel pointers
  std::array<float*, MaxLanes * 2> silence_ptrs;
  std::vector<std::vector<float>> silence(MaxLanes * 2, std::vector<float>(max_frame_count, 0.0f));
  for(int lc = 0; lc < MaxLanes * 2; lc++)
    silence_ptrs[lc] = silence[lc].data();
  juce::dsp::AudioBlock<float> block(silence_ptrs.data(), MaxLanes * 2, 0, max_frame_count);
  auto x2up = _2x.processSamplesUp(block);
  auto x4up = _4x.processSamplesUp(block);
  auto x8up = _8x.processSamplesUp(block);
  for (int lc = 0; lc < MaxLanes * 2; lc++)
  {
    _1x_lanes_channels_ptrs[lc] = _1x[lc].data().data();
    _2x_lanes_channels_ptrs[lc] = x2up.getChannelPointer(lc);
    _4x_lanes_channels_ptrs[lc] = x4up.getChannelPointer(lc);
    _8x_lanes_channels_ptrs[lc] = x8up.getChannelPointer(lc);
  }
}

template <int MaxLanes> float** 
oversampler<MaxLanes>::get_upsampled_lanes_channels_ptrs(int factor)
{
  switch (factor)
  {
  case 1: return _1x_lanes_channels_ptrs.data();
  case 2: return _2x_lanes_channels_ptrs.data();
  case 4: return _4x_lanes_channels_ptrs.data();
  case 8: return _8x_lanes_channels_ptrs.data();
  default: assert(false); return nullptr;
  }
}

template <int MaxLanes>
template <class NonLinear> void
oversampler<MaxLanes>::process_off(
  std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear)
{
  float* data[MaxLanes * 2] = { nullptr };
  for (int l = 0; l < active_lanes; l++)
  {
    data[l * 2 + 0] = (*inout[l])[0].data().data();
    data[l * 2 + 1] = (*inout[l])[1].data().data();
  }

  // upsample -> copy
  float** not_upsampled = get_upsampled_lanes_channels_ptrs(1);
  if (upsample)
    for(int lc = 0; lc < active_lanes * 2; lc++)
      for(int f = 0; f < end_frame - start_frame; f++)
        not_upsampled[lc][f] = data[lc][start_frame + f];
  for (int f = 0; f < end_frame - start_frame; f++)
    non_linear(not_upsampled, f);
  for (int lc = 0; lc < active_lanes * 2; lc++)
    for (int f = 0; f < end_frame - start_frame; f++)
      data[lc][start_frame + f] = not_upsampled[lc][f];
}

template <int MaxLanes>
template <int Factor, class NonLinear> void
oversampler<MaxLanes>::process(
  juce::dsp::Oversampling<float>& oversampling, 
  std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear)
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
  if(upsample) oversampling.processSamplesUp(inout_block);
  float** upsampled = get_upsampled_lanes_channels_ptrs(Factor);
  for (int f = 0; f < block_size * Factor; f++)
    non_linear(upsampled, f);
  oversampling.processSamplesDown(inout_block);
}

template <int MaxLanes>
template <class NonLinear> void
oversampler<MaxLanes>::process(
  int stages, std::array<jarray<float, 2>*, MaxLanes> const& inout,
  int active_lanes, int start_frame, int end_frame, bool upsample, NonLinear non_linear)
{
  using juce::dsp::AudioBlock;
  assert(0 <= stages && stages <= 3);
  switch (stages)
  {
  case 0: process_off(inout, active_lanes, start_frame, end_frame, upsample, non_linear); break;
  case 1: process<2>(_2x, inout, active_lanes, start_frame, end_frame, upsample, non_linear); break;
  case 2: process<4>(_4x, inout, active_lanes, start_frame, end_frame, upsample, non_linear); break;
  case 3: process<8>(_8x, inout, active_lanes, start_frame, end_frame, upsample, non_linear); break;
  default: assert(false); break;
  }
}

}
