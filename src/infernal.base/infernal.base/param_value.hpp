#pragma once

namespace infernal::base {

struct param_topo;

struct param_value final { 
  union { float real; int step; };
  
  static param_value from_step(int step)
  {
    param_value result;
    result.step = step;
    return result;
  }

  static param_value from_real(float real)
  {
    param_value result;
    result.real = real;
    return result;
  }
};

}
