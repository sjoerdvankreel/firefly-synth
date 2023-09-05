#pragma once
#include <infernal.base/topo.hpp>
#include <cmath>

namespace infernal::base {

struct param_value final { 
  union { float real; int step; };
  
  double to_plain(param_topo const& topo) const;
  double to_normalized(param_topo const& topo) const;
  std::string to_text(param_topo const& topo) const;

  static param_value default_value(param_topo const& topo);
  static double default_normalized(param_topo const& topo);

  static param_value from_step(int step);
  static param_value from_real(float real);
  static param_value from_plain(param_topo const& topo, double plain);
  static param_value from_normalized(param_topo const& topo, double normalized);
  static bool from_text(param_topo const& topo, std::string const& text, param_value& value);
};

}
