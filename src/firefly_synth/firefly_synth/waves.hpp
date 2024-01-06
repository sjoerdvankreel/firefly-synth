#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/helpers/multi_menu.hpp>

#include <cmath>
#include <cassert>

// waveforms for lfos and shapers
namespace firefly_synth
{

inline float const pi32 = plugin_base::pi32;

plugin_base::multi_menu make_wave_multi_menu();
std::vector<plugin_base::topo_tag> wave_skew_type_tags();
std::vector<plugin_base::topo_tag> wave_shape_type_tags();

std::string wave_make_header(int skew_x, int skew_y);
std::string wave_make_name(int skew_x, int skew_y, int shape);

enum { wave_skew_type_off, wave_skew_type_lin, wave_skew_type_exp };
enum { wave_shape_type_saw, wave_shape_type_sqr, wave_shape_type_sin };

inline float wave_skew_off(float in, float p) { return in; }
inline float wave_skew_exp(float in, float p) { return std::pow(in, p); }
inline float wave_skew_lin(float in, float p) { return in == p? in: in < p? in / p * 0.5f: 0.5f + (in - p) / (1 - p) * 0.5f; }

inline float wave_shape_saw(float in) { return in; }
inline float wave_shape_sqr(float in) { return in < 0.5f? 0.0f: 1.0f; }
inline float wave_shape_sin(float in) { return plugin_base::bipolar_to_unipolar(std::sin(in * 2 * pi32)); }

template <class SkewX, class SkewY, class Shape>
inline float wave_calc_unipolar(float in, float x, float y, SkewX skew_x, SkewY skew_y, Shape shape)
{
  plugin_base::check_unipolar(in);
  //plugin_base::check_unipolar(x);
  //plugin_base::check_unipolar(y);
  float skewed_in = plugin_base::check_unipolar(skew_x(in, x));
  float shaped = plugin_base::check_unipolar(shape(skewed_in));
  return plugin_base::check_unipolar(skew_y(shaped, y));
}

}