#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <sstream>
#include <iomanip>

namespace infernal::base {

double
param_value::default_normalized(param_topo const& topo)
{
  param_value result(default_value(topo));
  return result.to_normalized(topo);
}

param_value 
param_value::default_value(param_topo const& topo)
{
  param_value result;
  INF_ASSERT_EXEC(from_text(topo, topo.default_text, result));
  return result;
}

double 
param_value::to_plain(param_topo const& topo) const
{
  if(topo.config.format == param_format::step)
    return step;
  else
    return real;
}

param_value 
param_value::from_plain(param_topo const& topo, double plain)
{
  if (topo.config.format == param_format::step)
    return from_step(plain);
  else
    return from_real(plain);
}

std::string 
param_value::to_text(param_topo const& topo) const
{
  int prec = topo.percentage ? 3 : 5;
  int mult = topo.percentage ? 100 : 1;
  std::ostringstream stream;
  switch (topo.config.format)
  {
  case param_format::step:
    if(topo.config.display == param_display::toggle)
      stream << (step == 0? "Off": "On");
    else if(topo.config.display == param_display::list)
      stream << topo.list[step].name;
    else
      stream << step;
    break;
  case param_format::log:
  case param_format::linear: 
    stream << std::setprecision(prec) << real * (mult);
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
  int mult = topo.percentage ? 100 : 1;
  std::istringstream stream(text);
  switch (topo.config.format)
  {
  case param_format::step:
    value.step = std::numeric_limits<int>::max();
    if (topo.config.display == param_display::toggle)
    {
      if(text == "Off")
        value.step = 0;
      else if(text == "On")
        value.step = 1;
    }
    else if(topo.config.display == param_display::list)
    {
      for(int i = 0; i < topo.list.size(); i++)
        if(topo.list[i].name == text)
          value.step = i;
    } else
      stream >> value.step;
    return topo.min <= value.step && value.step <= topo.max;
  case param_format::log:
  case param_format::linear:
    value.real = std::numeric_limits<float>::max();
    stream >> value.real;
    value.real /= mult;
    return topo.min <= value.real && value.real <= topo.max;
  default:
    assert(false);
    return false;
  }
}

}
