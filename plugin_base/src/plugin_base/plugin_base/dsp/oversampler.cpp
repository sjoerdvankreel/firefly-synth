#include <plugin_base/dsp/oversampler.hpp>

using namespace juce::dsp;

namespace plugin_base {

static Oversampling<float>::FilterType 
filter_type(bool iir) 
{ 
  if(iir) return Oversampling<float>::filterHalfBandPolyphaseIIR;
  return Oversampling<float>::filterHalfBandFIREquiripple; 
}

oversampler::
oversampler(int max_frame_count, bool iir, bool max_quality, bool integer_latency):
_max_frame_count(max_frame_count),
_1x(2, 0, filter_type(iir), max_quality, integer_latency),
_2x(2, 1, filter_type(iir), max_quality, integer_latency),
_4x(2, 2, filter_type(iir), max_quality, integer_latency),
_8x(2, 3, filter_type(iir), max_quality, integer_latency),
_16x(2, 4, filter_type(iir), max_quality, integer_latency) 
{
  _1x.initProcessing(max_frame_count);
  _2x.initProcessing(max_frame_count);
  _4x.initProcessing(max_frame_count);
  _8x.initProcessing(max_frame_count);
  _16x.initProcessing(max_frame_count);
}

}