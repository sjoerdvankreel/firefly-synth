#pragma once

#include <limits>
#include <cassert>

namespace plugin_base {

// in [0, 1], 0.5 is log midpoint
class normalized_value final {
  double _value;
public:
  double value() const { return _value; }
  normalized_value() : _value(0.0) {}
  normalized_value(normalized_value const&) = default;
  normalized_value& operator=(normalized_value const&) = default;
  explicit normalized_value(double value) : _value(value) 
  { assert(0 <= value && value <= 1); }
};

// linear with min/max
class plain_value final {
  friend struct param_topo;
  union { float _real; int _step; };

  static plain_value from_real(float real);
  static plain_value from_step(int step) 
  { plain_value v; v._step = step; return v; }

public:
  int step() const { return _step; }
  float real() const { return _real; }
  void real_unchecked(float raw) { _real = raw; }

  plain_value() : _real(0.0f) {}
  plain_value(plain_value const&) = default;
  plain_value& operator=(plain_value const&) = default;
};

inline plain_value 
plain_value::from_real(float real)
{
  assert(real == real);
  assert(real < std::numeric_limits<float>::infinity());
  assert(real > -std::numeric_limits<float>::infinity());
  plain_value v; 
  v._real = real; 
  return v;
}

}
