#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <juce_dsp/juce_dsp.h>
#include <cassert>

namespace plugin_base {

class oversampler {
  int const _max_frame_count;
  juce::dsp::Oversampling<float> _1x;
  juce::dsp::Oversampling<float> _2x;
  juce::dsp::Oversampling<float> _4x;
  juce::dsp::Oversampling<float> _8x;
  juce::dsp::Oversampling<float> _16x;
public:
  oversampler(int max_frame_count, bool iir, bool max_quality, bool integer_latency);
  template <class NonLinear>
  void process(int stages, jarray<float, 2>& inout, int start_frame, int end_frame, NonLinear non_linear);
};

template <class NonLinear> void
oversampler::process(int stages, jarray<float, 2>& inout, int start_frame, int end_frame, NonLinear non_linear)
{
  using juce::dsp::AudioBlock;
  assert(0 <= stages && stages <= 4);

  int factor = 1 << stages;
  int block_size = end_frame - start_frame;
  if (stages == 0)
  {
    float* data[2];
    data[0] = inout[0].data().data();
    data[1] = inout[1].data().data();
    AudioBlock<float> inout_block(data, 2, start_frame, block_size);
    auto up_block = _1x.processSamplesUp(inout_block);
    for(int c = 0; c < 2; c++)
    {
      float* p = up_block.getChannelPointer(c);
      for(int f = 0; f < block_size * factor; f++)
        p[f] = non_linear(p[f]);
    }
    _1x.processSamplesDown(inout_block);
  }
}

}
