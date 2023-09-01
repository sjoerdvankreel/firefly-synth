#pragma once

namespace infernal::base {

struct param_topo;

struct param_value final { 
  union { float real; int step; };
};

}
