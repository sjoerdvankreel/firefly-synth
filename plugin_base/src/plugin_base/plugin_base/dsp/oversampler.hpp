#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cassert>

namespace plugin_base {

class oversampler {
  
  int const _max_lanes;
  int const _max_frame_count;

  juce::dsp::Oversampling<float> _1x;
  juce::dsp::Oversampling<float> _2x;
  juce::dsp::Oversampling<float> _4x;
  juce::dsp::Oversampling<float> _8x;

  std::vector<float**> _lanes_ptrs_down;
  std::vector<float**> _1x_lanes_ptrs_up;
  std::vector<float**> _2x_lanes_ptrs_up;
  std::vector<float**> _4x_lanes_ptrs_up;
  std::vector<float**> _8x_lanes_ptrs_up;
  std::vector<float*> _channels_ptrs_down;
  std::vector<float*> _1x_channels_ptrs_up;
  std::vector<float*> _2x_channels_ptrs_up;
  std::vector<float*> _4x_channels_ptrs_up;
  std::vector<float*> _8x_channels_ptrs_up;

  void init_ptrs_down(int lanes, float*** ptrs);

public:
  PB_PREVENT_ACCIDENTAL_COPY(oversampler);
  oversampler(int max_lanes, int max_frame_count, bool iir, bool max_quality, bool integer_latency);

  // set upsample process to false if not using the buffer content, 
  // in that case we just return garbage memory to fill by the caller
  // and that can later be downsampled
  void downsample(int stages, int lanes, int start_frame, int end_frame, float*** output);
  float*** upsample(int stages, int lanes, int start_frame, int end_frame, float const*** input, bool process);
};

}
