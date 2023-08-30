#include <infernal.base.vst3/vst3_support.hpp>

#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

void
copy_to_vst_string(TChar* dest, int count, char const* source)
{
  memset(dest, 0, sizeof(*dest) * count);
  for (int i = 0; i < count - 1 && i < strlen(source); i++)
    dest[i] = source[i];
}

param_value 
denormalize(param_topo const& topo, double value)
{
  param_value result;
  if(topo.format == param_format::real) result.real = value;
  else result.step = topo.stepped_min + value * (topo.stepped_max - topo.stepped_min + 1);
  return result;
}

double 
normalize(param_topo const& topo, param_value value)
{
  if(topo.format == param_format::real) return value.real;
  return ((float)value.step - topo.stepped_min) / (topo.stepped_max - topo.stepped_min);
}

}