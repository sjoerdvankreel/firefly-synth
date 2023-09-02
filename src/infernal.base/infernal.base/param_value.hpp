#pragma once
#include <infernal.base/topo.hpp>

namespace infernal::base {

struct param_value final { 
  union { float real; int step; };
  
  std::string to_text(param_topo const& topo) const;
  double to_normalized(param_topo const& topo) const;

  static param_value from_step(int step);
  static param_value from_real(float real);
  static param_value default_value(param_topo const& topo);
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
param_value::to_normalized(param_topo const& topo) const
{
  switch (topo.format)
  {
  case param_format::log:
  case param_format::linear: return (real - topo.min) / (topo.max - topo.min);
  case param_format::step: return (step - topo.min) / (topo.max - topo.min);
  default: assert(false); return 0;
  }
}

inline param_value
param_value::from_normalized(param_topo const& topo, double normalized)
{
  switch (topo.format)
  {
  case param_format::log:
  case param_format::linear: return from_real(topo.min + normalized * (topo.max - topo.min));
  case param_format::step: return from_step(topo.min + normalized * (topo.max - topo.min));
  default: assert(false); return {};
  }
}

}
