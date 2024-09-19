#pragma once

#include <plugin_base/dsp/utility.hpp>

#include <array>
#include <cassert>
#include <cstdint>

// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/procedural-patterns-noise-part-1/creating-simple-1D-noise.html
namespace firefly_synth
{

inline float
lerp(float lo, float hi, float t) 
{ return lo * (1 - t) + hi * t; }

inline float 
cosine_remap(float a, float b, float t) 
{ 
  assert(t >= 0 && t <= 1); 
  return lerp(a, b, (1 - std::cos(t * plugin_base::pi32)) * 0.5f);
}

template <bool Smooth>
class noise_generator
{
  static int const MAX_STEPS = 100;
  int _steps = -1;
  std::uint32_t _state = 0;
  std::array<float, MAX_STEPS> _r = {};

public:
  float at(float phase) const;
  void init(int seed, int steps);
  
  // returns seed to reproduce state for gui
  // TODO this is not sound -- from this we are feeding the gui an entire new table which is seeded from the *end* of the audio table
  void sample_table(std::uint32_t state);
  std::uint32_t sample_table(bool connect);
  std::uint32_t state() const { return _state; }

  noise_generator() {} // needs init
  noise_generator(int seed, int steps) { init(seed, steps); }
};

template <bool Smooth> inline void 
noise_generator<Smooth>::init(int seed, int steps)
{
  _steps = std::clamp(steps, 2, MAX_STEPS);
  _state = plugin_base::fast_rand_seed(seed);
  sample_table(false);
}

template <bool Smooth> inline void
noise_generator<Smooth>::sample_table(std::uint32_t state)
{
  _state = state;
  sample_table(true);
}

template <bool Smooth> inline std::uint32_t
noise_generator<Smooth>::sample_table(bool connect)
{
  auto result = _state;
  if constexpr (Smooth)
  {
    if (connect)
    {
      // make it smooth - a bunch of unrelated cosine remapped functions
      // stacked next to each other are not smooth. needs a bit of juggling.
      // TODO start of the signal is defined by connecting [0] and [1]
      // but for true random smooth noise cannot just keep [0] intact all the time
      // but if i swap it then discontinuity -- what to do ?
      // anyway this works
      for (int i = 1; i < _steps; ++i)
        _r[i] = plugin_base::fast_rand_next(_state);
    }
    else
    {
      for (int i = 0; i < _steps; ++i)
        _r[i] = plugin_base::fast_rand_next(_state);
    }
  }
  else
  {
    for (int i = 0; i < _steps; ++i)
      _r[i] = plugin_base::fast_rand_next(_state);
  }
  return result;
}

template <bool Smooth> inline float
noise_generator<Smooth>::at(float phase) const
{
  // failed to init()
  assert(_steps >= 2);

  float x = phase * _steps;
  int xi = (int)x - (x < 0 && x != (int)x);
  int x_min = xi % _steps;
  if constexpr (!Smooth)
    return _r[x_min];
  else
  {
    int x_max = (x_min + 1) % _steps;
    float t = x - xi;
    return cosine_remap(_r[x_min], _r[x_max], t);
  }
}

}