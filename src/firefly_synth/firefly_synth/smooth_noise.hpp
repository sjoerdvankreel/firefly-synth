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
smooth_step_remap(float a, float b, float t)
{
  assert(t >= 0 && t <= 1);
  return lerp(a, b, t * t * (3 - 2 * t));
}

inline float 
cosine_remap(float a, float b, float t) 
{ 
  assert(t >= 0 && t <= 1); 
  return lerp(a, b, (1 - std::cos(t * plugin_base::pi32)) * 0.5f);
}

template <float Remap(float, float, float)>
class smooth_noise
{
  std::vector<float> _r;
  std::uint32_t _state = 0;
  std::uint32_t const _steps;

public:
  float next(float x);
  smooth_noise(int seed, std::uint32_t steps);
};

template <float Remap(float, float, float)>
inline smooth_noise<Remap>::
smooth_noise(int seed, std::uint32_t steps):
_steps(steps), _r(steps, 0.0f)
{
  _state = plugin_base::fast_rand_seed(seed);
  for (int i = 0; i < steps; ++i)
    _r[i] = plugin_base::fast_rand_next(_state);
}

template <float Remap(float, float, float)>
inline float 
smooth_noise<Remap>::next(float x)
{
  int xi = (int)x - (x < 0 && x != (int)x);
  float t = x - xi;
  int x_min = xi & (_steps - 1);
  int x_max = (x_min + 1) & (_steps - 1);
  return Remap(_r[x_min], _r[x_max], t);
}

}