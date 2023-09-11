#include <plugin_base/utility.hpp>
#include <plugin_base/param_value.hpp>
#include <sstream>
#include <iomanip>

namespace plugin_base {

// all defaults are text so they can be hardcoded as shown in the ui
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
  if(topo.unit.size()) stream << " " << topo.unit;
  return stream.str();
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
  if(topo.type == param_type::linear)
    value.real /= topo.percentage ? 100 : 1;
  else if (topo.type == param_type::log)
    value.real = value.to_normalized(topo);
  else assert(false);
  return topo.min <= value.real && value.real <= topo.max;
}

}
