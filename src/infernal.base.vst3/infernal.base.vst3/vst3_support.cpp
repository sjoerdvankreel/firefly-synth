#include <infernal.base.vst3/vst3_support.hpp>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

void
to_vst_string(TChar* dest, int count, char const* source)
{
  memset(dest, 0, sizeof(*dest) * count);
  for (int i = 0; i < count - 1 && i < strlen(source); i++)
    dest[i] = source[i];
}

param_value 
denormalize(param_topo const& topo, double value)
{
  param_value result;
  if(topo.format != param_format::step) result.real = value;
  else result.step = topo.min + value * (topo.max - topo.min + 1);
  return result;
}

double 
normalize(param_topo const& topo, param_value value)
{
  if(topo.format != param_format::step) return value.real;
  return (value.step - topo.min) / (topo.max - topo.min);
}

}