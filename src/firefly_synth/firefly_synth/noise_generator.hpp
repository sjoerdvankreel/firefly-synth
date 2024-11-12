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

inline int constexpr noise_generator_max_steps = 100;

template <bool Smooth>
class noise_generator
{
  int _seed = 0;
  int _steps = 2;
  std::uint32_t _state = 0;
  std::array<float, noise_generator_max_steps> _r = {};

public:
  float at(float phase) const;
  void init(int seed, int steps);
  int seed() const { return _seed; }
  void resample() { init(_state, _steps); }; // for free-run

  noise_generator() {} // needs init
  noise_generator(int seed, int steps) 
  { init(plugin_base::fast_rand_seed(seed), steps); }
};

template <bool Smooth> inline void 
noise_generator<Smooth>::init(int seed, int steps)
{
  _seed = seed;
  _state = seed;
  _steps = std::clamp(steps, 2, noise_generator_max_steps);
  for (int i = 0; i < _steps; ++i)
    _r[i] = plugin_base::fast_rand_next(_state);
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