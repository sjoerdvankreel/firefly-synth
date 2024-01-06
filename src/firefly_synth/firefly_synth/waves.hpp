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

std::string wave_make_header(int skew_x, int skew_y);
std::string wave_make_name(int skew_x, int skew_y, int shape);

enum { wave_skew_type_off, wave_skew_type_scu, wave_skew_type_scb, wave_skew_type_lin, wave_skew_type_exp };
enum { wave_shape_type_saw, wave_shape_type_sqr, wave_shape_type_sin };

inline float wave_skew_off(float in, float p) { return in; }
inline float wave_skew_scu(float in, float p) { return in * p; }
inline float wave_skew_scb(float in, float p) { return bipolar_to_unipolar(unipolar_to_bipolar(in) * p); }
inline float wave_skew_lin(float in, float p) { return in == p? in: in < p? in / p * 0.5f: 0.5f + (in - p) / (1 - p) * 0.5f; }
inline float wave_skew_exp(float in, float p) { return std::pow(in, p); }

inline float wave_shape_saw(float in) { return in; }
inline float wave_shape_sqr(float in) { return in < 0.5f? 0.0f: 1.0f; }
inline float wave_shape_sin(float in) { return plugin_base::bipolar_to_unipolar(std::sin(in * 2 * pi32)); }

template <class SkewX, class SkewY, class Shape>
inline float wave_calc_unipolar(float in, float x, float y, SkewX skew_x, SkewY skew_y, Shape shape)
{
  using plugin_base::check_unipolar;
  check_unipolar(in);
  float skewed_in = check_unipolar(skew_x(in, x));
  float shaped = check_unipolar(shape(skewed_in));
  return check_unipolar(skew_y(shaped, y));
}

}