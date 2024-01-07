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

plugin_base::multi_menu make_wave_multi_menu(bool for_shaper);

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
  wave_shape_type_smooth_or_fold, wave_shape_type_static, wave_shape_type_static_free };

inline bool wave_skew_is_exp(int skew) { return skew == wave_skew_type_xpu || skew == wave_skew_type_xpb; }

inline float wave_skew_uni_off(float in, float p) { return in; }
inline float wave_skew_uni_lin(float in, float p) { return in == p ? in : in < p ? in / p * 0.5f : 0.5f + (in - p) / (1 - p) * 0.5f; }
inline float wave_skew_uni_scu(float in, float p) { return in * p; }
inline float wave_skew_uni_scb(float in, float p) { return bipolar_to_unipolar(unipolar_to_bipolar(in) * p); }
inline float wave_skew_uni_xpu(float in, float p) { return std::pow(in, p); }
inline float wave_skew_uni_xpb(float in, float p) { 
  float bp = unipolar_to_bipolar(in); 
  return bipolar_to_unipolar((bp < 0? -1: 1) * std::pow(std::fabs(bp), p)); }

inline float wave_skew_bi_off(float in, float p) { return in; }
inline float wave_skew_bi_lin(float in, float p) { 
  float bp = unipolar_to_bipolar(p); 
  return (in < -1 || in > 1 || in == bp)? in: in < bp ? -1 + (in - -1) / (bp - -1): (in - bp) / (1 - bp); }
inline float wave_skew_bi_scu(float in, float p) { return in * p; }
inline float wave_skew_bi_scb(float in, float p) { return in * unipolar_to_bipolar(p); }
inline float wave_skew_bi_xpu(float in, float p) { return in; }
inline float wave_skew_bi_xpb(float in, float p) { return (in < 0 ? -1 : 1) * std::pow(std::fabs(in), p); }

inline float wave_shape_uni_saw(float in) { return in; }
inline float wave_shape_uni_sqr(float in) { return in < 0.5f? 0.0f: 1.0f; }
inline float wave_shape_uni_tri(float in) { return 1 - std::fabs(unipolar_to_bipolar(in)); }
inline float wave_shape_uni_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32)); }
inline float wave_shape_uni_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32)); }
inline float wave_shape_uni_sin_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32))); }
inline float wave_shape_uni_sin_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32))); }
inline float wave_shape_uni_cos_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32))); }
inline float wave_shape_uni_cos_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32))); }
inline float wave_shape_uni_sin_sin_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_uni_sin_sin_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_uni_sin_cos_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_uni_sin_cos_cos(float in) { return bipolar_to_unipolar(std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_uni_cos_sin_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_uni_cos_sin_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }
inline float wave_shape_uni_cos_cos_sin(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::sin(in * 2 * pi32)))); }
inline float wave_shape_uni_cos_cos_cos(float in) { return bipolar_to_unipolar(std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32 + std::cos(in * 2 * pi32)))); }

inline float wave_shape_bi_saw(float in) { return in; }
inline float wave_shape_bi_sqr(float in) { return in < 0 ? -1.0f : 1.0f; }
inline float wave_shape_bi_tri(float in) { return in < -1? in: in > 1? in: unipolar_to_bipolar(1 - std::fabs(in)); }
inline float wave_shape_bi_sin(float in) { return std::sin(in * pi32); }
inline float wave_shape_bi_cos(float in) { return std::cos(in * pi32); }
inline float wave_shape_bi_sin_sin(float in) { return std::sin(in * pi32 + std::sin(in * pi32)); }
inline float wave_shape_bi_sin_cos(float in) { return std::sin(in * pi32 + std::cos(in * pi32)); }
inline float wave_shape_bi_cos_sin(float in) { return std::cos(in * pi32 + std::sin(in * pi32)); }
inline float wave_shape_bi_cos_cos(float in) { return std::cos(in * pi32 + std::cos(in * pi32)); }
inline float wave_shape_bi_sin_sin_sin(float in) { return std::sin(in * pi32 + std::sin(in * pi32 + std::sin(in * pi32))); }
inline float wave_shape_bi_sin_sin_cos(float in) { return std::sin(in * pi32 + std::sin(in * pi32 + std::cos(in * pi32))); }
inline float wave_shape_bi_sin_cos_sin(float in) { return std::sin(in * pi32 + std::cos(in * pi32 + std::sin(in * pi32))); }
inline float wave_shape_bi_sin_cos_cos(float in) { return std::sin(in * pi32 + std::cos(in * pi32 + std::cos(in * pi32))); }
inline float wave_shape_bi_cos_sin_sin(float in) { return std::cos(in * pi32 + std::sin(in * pi32 + std::sin(in * pi32))); }
inline float wave_shape_bi_cos_sin_cos(float in) { return std::cos(in * pi32 + std::sin(in * pi32 + std::cos(in * pi32))); }
inline float wave_shape_bi_cos_cos_sin(float in) { return std::cos(in * pi32 + std::cos(in * pi32 + std::sin(in * pi32))); }
inline float wave_shape_bi_cos_cos_cos(float in) { return std::cos(in * pi32 + std::cos(in * pi32 + std::cos(in * pi32))); }

template <class Custom>
inline float wave_shape_uni_custom(float in, Custom custom) { return custom(in); }

inline float
wave_shape_bi_fold(float in)
{
  while (true)
    if (in > 1.0f) in -= 2.0f * (in - 1.0f);
    else if (in < -1.0f) in += 2.0f * (-in - 1.0f);
    else return in;
  assert(false);
  return 0.0f;
}

template <class Shape, class SkewX, class SkewY>
inline float wave_calc_uni(float in, float x, float y, Shape shape, SkewX skew_x, SkewY skew_y)
{
  using plugin_base::check_unipolar;
  check_unipolar(in);
  float skewed_in = check_unipolar(skew_x(in, x));
  float shaped = check_unipolar(shape(skewed_in));
  return check_unipolar(skew_y(shaped, y));
}

template <class Shape, class SkewX, class SkewY>
inline float wave_calc_bi(float in, float x, float y, Shape shape, SkewX skew_x, SkewY skew_y)
{
  float skewed_in = skew_x(in, x);
  float shaped = shape(skewed_in);
  return skew_y(shaped, y);
}

}