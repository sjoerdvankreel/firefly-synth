#include <infernal.base/topo.hpp>
#include <sstream>
#include <iomanip>

namespace infernal::base {

param_value 
param_topo::default_value() const
{
  param_value result;
  INF_ASSERT_EXEC(from_text(default_text, result));
  return result;
}

std::string 
param_topo::to_text(param_value value) const
{
  std::ostringstream stream;
  switch (format)
  {
  case param_format::step:
    stream << value.step;
    break;
  case param_format::log:
  case param_format::linear: 
    stream << std::setprecision(precision) << value.real;
    break;
  default:
    assert(false);
    break;
  }
  std::string result = stream.str();
  if(unit.size() > 0) result += " " + unit;
  return result;
}

bool 
param_topo::from_text(std::string const& text, param_value& value) const
{
  std::istringstream stream(text);
  switch (format)
  {
  case param_format::step:
    value.step = std::numeric_limits<int>::max();
    stream >> value.step;
    return min <= value.step && value.step <= max;
  case param_format::log:
  case param_format::linear:
    value.real = std::numeric_limits<float>::max();
    stream >> value.real;
    return min <= value.real && value.real <= max;
  default:
    assert(false);
    return false;
  }
}

}
