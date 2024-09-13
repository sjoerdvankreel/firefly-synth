#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <cstdint>

namespace firefly_synth
{

class static_noise
{
  float _level = 0;
  int _step_pos = 0;
  int _total_pos = 0;
  int _step_samples = 0;
  int _total_samples = 0;
  std::uint32_t _state = 1;

public:
  void reset(int seed);
  void update(float sr, float rate, int steps);
  template <bool Free> float next(float phase, int seed);
};

inline void
static_noise::reset(int seed)
{
  _step_pos = 0;
  _total_pos = 0;
  _state = plugin_base::fast_rand_seed(seed);
  _level = plugin_base::fast_rand_next(_state);
}

inline void 
static_noise::update(float sr, float rate, int steps)
{
  _total_samples = std::ceil(sr / rate);
  _step_samples = std::ceil(sr / (rate * steps));
}

template <bool Free> inline float
static_noise::next(float phase, int seed)
{
  float result = _level;
  _step_pos++;
  _total_pos++;
  if (_step_pos >= _step_samples)
  {
    // just do something with the phase param so we don't need to filter out the X options
    _level = plugin_base::fast_rand_next(_state);
    _level = bipolar_to_unipolar(unipolar_to_bipolar(_level) * phase);
    _step_pos = 0;
  }
  if constexpr (!Free)
    if (_total_pos >= _total_samples)
      reset(seed);
  return result;
}

}