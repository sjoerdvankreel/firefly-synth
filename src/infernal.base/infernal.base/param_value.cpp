#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <sstream>
#include <iomanip>

namespace infernal::base {

param_value 
param_value::default_value(param_topo const& topo)
{
  param_value result;
  INF_ASSERT_EXEC(from_text(topo, topo.default_text, result));
  return result;
}

std::string 
param_value::to_text(param_topo const& topo) const
{
  std::ostringstream stream;
  switch (topo.format)
  {
  case param_format::step:
    stream << step;
    break;
  case param_format::log:
  case param_format::linear: 
    stream << std::setprecision(topo.precision) << real;
    break;
  default:
    assert(false);
    break;
  }
  std::string result = stream.str();
  if(topo.unit.size() > 0) result += " " + topo.unit;
  return result;
}

bool 
param_value::from_text(param_topo const& topo, std::string const& text, param_value& value)
{
  std::istringstream stream(text);
  switch (topo.format)
  {
  case param_format::step:
    value.step = std::numeric_limits<int>::max();
    stream >> value.step;
    return topo.min <= value.step && value.step <= topo.max;
  case param_format::log:
  case param_format::linear:
    value.real = std::numeric_limits<float>::max();
    stream >> value.real;
    return topo.min <= value.real && value.real <= topo.max;
  default:
    assert(false);
    return false;
  }
}

}
