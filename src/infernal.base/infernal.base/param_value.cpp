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
    if(topo.display == param_display::toggle)
      stream << (step == 0? "Off": "On");
    else if(topo.display == param_display::list)
      stream << topo.list[step].name;
    else
      stream << step;
    break;
  case param_format::log:
  case param_format::linear: 
    stream << std::setprecision(3) << real * (topo.percentage ? 1: 100);
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
    if (topo.display == param_display::toggle)
    {
      if(text == "Off")
        value.step = 0;
      else if(text == "On")
        value.step = 1;
    }
    else if(topo.display == param_display::list)
    {
      auto iter = std::find(topo.list.begin(), topo.list.end(), text);
      if(iter != topo.list.end())
        value.step = iter - topo.list.begin();
    } else
      stream >> value.step;
    return topo.min <= value.step && value.step <= topo.max;
  case param_format::log:
  case param_format::linear:
    value.real = std::numeric_limits<float>::max();
    stream >> value.real;
    if(topo.percentage) value.real /= 100;
    return topo.min <= value.real && value.real <= topo.max;
  default:
    assert(false);
    return false;
  }
}

}
