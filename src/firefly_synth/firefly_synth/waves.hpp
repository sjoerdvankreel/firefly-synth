#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/helpers/multi_menu.hpp>

#include <cmath>
#include <cassert>

// waveforms for lfos and shapers
namespace firefly_synth
{

using plugin_base::unipolar_to_bipolar;
using plugin_base::bipolar_to_unipolar;
inline float const pi32 = plugin_base::pi32;

plugin_base::multi_menu make_wave_multi_menu();
std::vector<plugin_base::topo_tag> wave_skew_type_tags();
std::vector<plugin_base::topo_tag> wave_shape_type_tags();

enum { wave_skew_type_off, wave_skew_type_lin, wave_skew_type_scu, wave_skew_type_scb, wave_skew_type_xpu, wave_skew_type_xpb };
enum { 
  wave_shape_type_saw, wave_shape_type_sqr, wave_shape_type_tri, 
  wave_shape_type_sin, wave_shape_type_cos,
  wave_shape_type_sin_sin, wave_shape_type_sin_cos,
  wave_shape_type_cos_sin, wave_shape_type_cos_cos,
  wave_shape_type_sin_sin_sin, wave_shape_type_sin_sin_cos,
  wave_shape_type_sin_cos_sin, wave_shape_type_sin_cos_cos,
  wave_shape_type_cos_sin_sin, wave_shape_type_cos_sin_cos,
  wave_shape_type_cos_cos_sin, wave_shape_type_cos_cos_cos,
  wave_shape_type_smooth };

inline bool wave_skew_is_exp(int skew) { return skew == wave_skew_type_xpu || skew == wave_skew_type_xpb; }

inline float wave_skew_off(float in, float p) { return in; }
inline float wave_skew_lin(float in, float p) { return in == p ? in : in < p ? in / p * 0.5f : 0.5f + (in - p) / (1 - p) * 0.5f; }
inline float wave_skew_scu(float in, float p) { return in * p; }
inline float wave_skew_scb(float in, float p) { return bipolar_to_unipolar(unipolar_to_bipolar(in) * p); }
inline float wave_skew_xpu(float in, float p) { return std::pow(in, p); }
inline float wave_skew_xpb(float in, float p) { float bp = unipolar_to_bipolar(in); return bipolar_to_unipolar((bp < 0? -1: 1) * std::pow(std::fabs(bp), p)); }

inline float wave_shape_saw(float in) { return in; }
inline float wave_shape_sqr(float in) { return in < 0.5f? 0.0f: 1.0f; }
inline float wave_shape_tri(float in) { return 1 - std::fabs(unipolar_to_bipolar(in)); }
inline float wave_shape_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32)); }
inline float wave_shape_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32)); }
inline float wave_shape_sin_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32))); }
inline float wave_shape_sin_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32))); }
inline float wave_shape_cos_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32))); }
inline float wave_shape_cos_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32))); }
inline float wave_shape_sin_sin_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_sin_sin_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_sin_cos_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_sin_cos_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_cos_sin_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_cos_sin_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_cos_cos_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_cos_cos_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
template <class Smooth> inline float wave_shape_smooth(float in, Smooth smooth) { return smooth(in); }

template <class Shape, class SkewX, class SkewY>
inline float wave_calc_unipolar(float in, float x, float y, Shape shape, SkewX skew_x, SkewY skew_y)
{
  using plugin_base::check_unipolar;
  check_unipolar(in);
  float skewed_in = check_unipolar(skew_x(in, x));
  float shaped = check_unipolar(shape(skewed_in));
  return check_unipolar(skew_y(shaped, y));
}

}