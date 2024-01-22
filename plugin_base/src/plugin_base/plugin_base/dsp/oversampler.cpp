#include <plugin_base/dsp/oversampler.hpp>

using namespace juce;
using namespace juce::dsp;

namespace plugin_base {

static Oversampling<float>::FilterType
filter_type(bool iir)
{
  if (iir) return Oversampling<float>::filterHalfBandPolyphaseIIR;
  return Oversampling<float>::filterHalfBandFIREquiripple;
}

oversampler::
oversampler(int max_lanes, int max_frame_count, bool iir, bool max_quality, bool integer_latency):
_max_lanes(max_lanes), _max_frame_count(max_frame_count), 
_1x(max_lanes * 2, 0, filter_type(iir), max_quality, integer_latency),
_2x(max_lanes * 2, 1, filter_type(iir), max_quality, integer_latency),
_4x(max_lanes * 2, 2, filter_type(iir), max_quality, integer_latency),
_8x(max_lanes * 2, 3, filter_type(iir), max_quality, integer_latency),
_lanes_ptrs_down(max_lanes, nullptr),
_1x_lanes_ptrs_up(max_lanes, nullptr), _2x_lanes_ptrs_up(max_lanes, nullptr),
_4x_lanes_ptrs_up(max_lanes, nullptr), _8x_lanes_ptrs_up(max_lanes, nullptr),
_channels_ptrs_down(max_lanes * 2, nullptr),
_1x_channels_ptrs_up(max_lanes * 2, nullptr), _2x_channels_ptrs_up(max_lanes * 2, nullptr),
_4x_channels_ptrs_up(max_lanes * 2, nullptr), _8x_channels_ptrs_up(max_lanes * 2, nullptr)
{
  assert(max_lanes > 0);
  assert(max_frame_count > 0);
  _1x.initProcessing(max_frame_count);
  _2x.initProcessing(max_frame_count);
  _4x.initProcessing(max_frame_count);
  _8x.initProcessing(max_frame_count);

  // feed the oversamplers some silence to get a hold of the channel pointers
  std::vector<std::vector<float>> silence(max_lanes * 2, std::vector<float>(max_frame_count, 0.0f));
  std::vector<float*> silence_ptrs(max_lanes * 2);
  for(int i = 0; i < max_lanes * 2; i++)
    silence_ptrs[i] = silence[i].data();
  AudioBlock<float> block(silence_ptrs.data(), max_lanes * 2, max_frame_count);
  auto x1 = _1x.processSamplesUp(block);
  auto x2 = _2x.processSamplesUp(block);
  auto x4 = _4x.processSamplesUp(block);
  auto x8 = _8x.processSamplesUp(block);
  for(int lane = 0; lane < max_lanes; lane++)
  {
    for (int ch = 0; ch < 2; ch++)
    {
      _1x_channels_ptrs_up[lane * 2 + ch] = x1.getChannelPointer(lane * 2 + ch);
      _2x_channels_ptrs_up[lane * 2 + ch] = x2.getChannelPointer(lane * 2 + ch);
      _4x_channels_ptrs_up[lane * 2 + ch] = x4.getChannelPointer(lane * 2 + ch);
      _8x_channels_ptrs_up[lane * 2 + ch] = x8.getChannelPointer(lane * 2 + ch);
    }
    _1x_lanes_ptrs_up[lane] = _1x_channels_ptrs_up.data() + lane * 2;
    _2x_lanes_ptrs_up[lane] = _2x_channels_ptrs_up.data() + lane * 2;
    _4x_lanes_ptrs_up[lane] = _4x_channels_ptrs_up.data() + lane * 2;
    _8x_lanes_ptrs_up[lane] = _8x_channels_ptrs_up.data() + lane * 2;
  }
}

void 
oversampler::init_ptrs_down(int lanes, float*** ptrs)
{
  for(int lane = 0; lane < lanes; lane++)
  {
    for(int ch = 0; ch < 2; ch++)
      _channels_ptrs_down[lane * 2 + ch] = ptrs[lane][ch];
    _lanes_ptrs_down[lane] = _channels_ptrs_down.data() + lane * 2;
  }
}

float***
oversampler::upsample(int stages, int lanes, int start_frame, int end_frame, float const*** input, bool process)
{
  assert(process == (input != nullptr));
  assert(lanes <= _max_lanes);
  assert(0 <= stages && stages <= 3);
  assert(0 < lanes && lanes <= _max_lanes);
  assert(start_frame >= 0);
  assert(start_frame <= end_frame && end_frame <= _max_frame_count);

  if (process)
  {
    init_ptrs_down(lanes, (float***)input);
    AudioBlock<float> block(_channels_ptrs_down.data(), lanes * 2, start_frame, end_frame - start_frame);
    switch (stages)
    {
    case 0: 
      for(int lane = 0; lane < lanes; lane++)
        for(int ch = 0; ch < 2; ch++)
          for(int f = start_frame; f < end_frame; f++)
            _1x_lanes_ptrs_up[lane][ch][f] = input[lane][ch][f];
      break;
    case 1: _2x.processSamplesUp(block); break;
    case 2: _4x.processSamplesUp(block); break;
    case 3: _8x.processSamplesUp(block); break;
    default: assert(false); break;
    }
  }

  switch (stages)
  {
  case 0: return _1x_lanes_ptrs_up.data();
  case 1: return _2x_lanes_ptrs_up.data();
  case 2: return _4x_lanes_ptrs_up.data();
  case 3: return _8x_lanes_ptrs_up.data();
  default: assert(false); return {};
  }
}

void
oversampler::downsample(int stages, int lanes, int start_frame, int end_frame, float*** output)
{
  assert(output);
  assert(lanes <= _max_lanes);
  assert(0 <= stages && stages <= 3);
  assert(0 < lanes && lanes <= _max_lanes);
  assert(start_frame >= 0);
  assert(start_frame <= end_frame && end_frame <= _max_frame_count);

  init_ptrs_down(lanes, output);
  AudioBlock<float> block(_channels_ptrs_down.data(), lanes * 2, start_frame, end_frame - start_frame);

  switch (stages)
  {
  case 0:
    for (int lane = 0; lane < lanes; lane++)
      for (int ch = 0; ch < 2; ch++)
        for (int f = start_frame; f < end_frame; f++)
          output[lane][ch][f] = _1x_lanes_ptrs_up[lane][ch][f];
    break;
  case 1: _2x.processSamplesDown(block); break;
  case 2: _4x.processSamplesDown(block); break;
  case 3: _8x.processSamplesDown(block); break;
  default: assert(false); break;
  }
}

}