#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <cmath>

// waveforms for lfos and shapers
namespace firefly_synth
{

inline float const pi32 = plugin_base::pi32;
std::string make_wave_id(int shape, int skew_x, int skew_y);
std::string make_wave_name(int shape, int skew_x, int skew_y);

// be sure to have these stable when adding/removing stuff as they are used for serialization
enum { wave_skew_type_off, wave_skew_type_lin, wave_skew_type_pow, wave_skew_type_count };
enum { wave_shape_type_saw, wave_shape_type_sqr, wave_shape_type_sin, wave_shape_type_count };

inline float wave_skew_off(float in, float p) { return in; }
inline float wave_skew_pow(float in, float p) { return std::pow(in, p); }
inline float wave_skew_lin(float in, float p) { return in < p ? in / p * 0.5f : 0.5f + (in - p) / (1 - p) * 0.5f; }

inline float wave_shape_saw(float in) { return in; }
inline float wave_shape_sqr(float in) { return in < 0.5? 0: 1; }
inline float wave_shape_sin(float in) { return std::sin(in * 2 * pi32); }

template <class Shape, class SkewX, class SkewY>
inline float wave_calc(float in, float px, float py, Shape shape, SkewX skew_x, SkewY skew_y)
{
  plugin_base::check_unipolar(in);
  plugin_base::check_unipolar(px);
  plugin_base::check_unipolar(py);
  return skew_y(shape(skew_x(in, px)), py);
}

}