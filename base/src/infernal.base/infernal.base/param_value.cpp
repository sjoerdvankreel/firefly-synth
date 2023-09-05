#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <sstream>
#include <iomanip>

namespace infernal::base {

param_value
param_value::from_step(int step)
{
  param_value result;
  result.step = step;
  return result;
}

param_value
param_value::from_real(float real)
{
  param_value result;
  result.real = real;
  return result;
}

double 
param_value::to_plain(param_topo const& topo) const
{
  if(topo.is_real())
    return real;
  else
    return step;
}

param_value 
param_value::from_plain(param_topo const& topo, double plain)
{
  if(topo.is_real())
    return from_real(plain);
  else
    return from_step(plain);
}

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
param_value::to_normalized(param_topo const& topo) const
{
  double range = topo.max - topo.min;
  if(!topo.is_real()) 
    return (step - topo.min) / range;
  switch (topo.type)
  {
  case param_type::log:
  case param_type::linear: return (real - topo.min) / range;
  default: assert(false); return 0;
  }
}

param_value
param_value::from_normalized(param_topo const& topo, double normalized)
{
  double range = topo.max - topo.min;
  if (!topo.is_real()) 
    return from_step(topo.min + std::floor(std::min(range, normalized * (range + 1))));
  switch (topo.type)
  {
  case param_type::log:
  case param_type::linear: return from_real(topo.min + normalized * range);
  default: assert(false); return {};
  }
}

std::string 
param_value::to_text(param_topo const& topo) const
{
  if(topo.display == param_display::toggle)
    return step == 0? "Off": "On";
  switch (topo.type)
  {
  case param_type::name: return topo.names[step];
  case param_type::item: return topo.items[step].name;
  case param_type::step: return std::to_string(step);
  default: break;
  }
      
  std::ostringstream stream;
  int prec = topo.percentage ? 3 : 5;
  int mult = topo.percentage ? 100 : 1;
  stream << std::setprecision(prec) << real * (mult);
  return stream.str() + " " + topo.unit;
}

bool 
param_value::from_text(param_topo const& topo, std::string const& text, param_value& value)
{
  if (topo.display == param_display::toggle)
  {
    if(text == "Off") value.step = 0;
    else if(text == "On") value.step = 1;
    else return false;
    return true;
  }

  if (topo.type == param_type::name)
  {
    for (int i = 0; i < topo.names.size(); i++)
      if (topo.names[i] == text)
      {
        value.step = i;
        return true;
      }
    return false;
  }

  if (topo.type == param_type::item)
  {
    for (int i = 0; i < topo.items.size(); i++)
      if (topo.items[i].name == text)
      {
        value.step = i;
        return true;
      }
    return false;
  }

  std::istringstream stream(text);
  if (topo.type == param_type::step)
  {
    value.step = std::numeric_limits<int>::max();
    stream >> value.step;
    return topo.min <= value.step && value.step <= topo.max;
  }

  value.real = std::numeric_limits<float>::max();
  stream >> value.real;
  value.real /= topo.percentage ? 100 : 1;
  return topo.min <= value.real && value.real <= topo.max;
}

}
