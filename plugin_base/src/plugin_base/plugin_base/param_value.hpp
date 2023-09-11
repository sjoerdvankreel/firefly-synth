#pragma once
#include <plugin_base/topo.hpp>
#include <cmath>

namespace plugin_base {

// internal parameter representation
// log scale is respected at all times
// plain (real) = real, plain (step) = step
struct param_value final { 
  union { float real; int step; };
  
  double to_plain(param_topo const& topo) const;
  double to_normalized(param_topo const& topo) const;
  std::string to_text(param_topo const& topo) const;
  static param_value default_value(param_topo const& topo);

  static param_value from_step(int step);
  static param_value from_real(float real);
  static param_value from_plain(param_topo const& topo, double plain);
  static param_value from_normalized(param_topo const& topo, double normalized);
  static bool from_text(param_topo const& topo, std::string const& text, param_value& value);
};

inline param_value
param_value::from_step(int step)
{
  param_value result;
  result.step = step;
  return result;
}

inline param_value
param_value::from_real(float real)
{
  param_value result;
  result.real = real;
  return result;
}

inline double
param_value::to_plain(param_topo const& topo) const
{
  if (topo.is_real())
    return real;
  else
    return step;
}

inline param_value
param_value::from_plain(param_topo const& topo, double plain)
{
  if (topo.is_real())
    return from_real(plain);
  else
    return from_step(plain);
}

inline double
param_value::to_normalized(param_topo const& topo) const
{
  double range = topo.max - topo.min;
  if (!topo.is_real())
    return (step - topo.min) / range;
  if (topo.type == param_type::linear)
    return (real - topo.min) / range;
  double log_range = topo.log_max - topo.log_min;
  double lin_real = topo.log_min + real * range;
  double result = std::pow((lin_real - topo.log_min) * (1 / log_range), 1 / topo.log_exp);
  return result;
}

inline param_value
param_value::from_normalized(param_topo const& topo, double normalized)
{
  double range = topo.max - topo.min;
  if (!topo.is_real())
    return from_step(topo.min + std::floor(std::min(range, normalized * (range + 1))));
  if (topo.type == param_type::linear)
    return from_real(topo.min + normalized * range);
  double log_range = topo.log_max - topo.log_min;
  double result = std::pow(normalized, topo.log_exp) * log_range + topo.log_min;
  return from_real(result);
}

}
