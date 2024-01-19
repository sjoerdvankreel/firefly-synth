#pragma once

#include <plugin_base/dsp/utility.hpp>

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

class smooth_noise
{
  std::uint32_t _steps;
  std::uint32_t _state = 0;
  std::vector<float> _r;

public:
  float next(float x);
  smooth_noise(int seed, int steps);
};

inline smooth_noise::
smooth_noise(int seed, int steps):
_steps(steps), _r(steps, 0.0f)
{
  _state = plugin_base::fast_rand_seed(seed);
  for (int i = 0; i < steps; ++i)
    _r[i] = plugin_base::fast_rand_next(_state);
}

inline float 
smooth_noise::next(float x)
{
  int xi = (int)x - (x < 0 && x != (int)x);
  float t = x - xi;
  int x_min = xi % _steps;
  int x_max = (x_min + 1) % _steps;
  return cosine_remap(_r[x_min], _r[x_max], t);
}

}