#include <plugin_base/topo.hpp>

#include <sstream>
#include <iomanip>

namespace plugin_base {

plain_value
param_topo::default_plain() const
{
  plain_value result;
  INF_ASSERT_EXEC(text_to_plain(default_, result));
  return result;
}

std::string 
param_topo::normalized_to_text(normalized_value normalized) const
{
  plain_value plain(normalized_to_plain(normalized));
  return plain_to_text(plain);
}

bool 
param_topo::text_to_normalized(
  std::string const& textual, normalized_value& normalized) const
{
  plain_value plain;
  if(!text_to_plain(textual, plain)) return false;
  normalized = plain_to_normalized(plain);
  return true;
}

std::string
param_topo::plain_to_text(plain_value plain) const
{
  if (edit == param_edit::toggle)
    return plain.step() == 0 ? "Off" : "On";
  switch (type)
  {
  case param_type::name: return names[plain.step()];
  case param_type::item: return items[plain.step()].name;
  case param_type::step: return std::to_string(plain.step());
  default: break;
  }

  std::ostringstream stream;
  int prec = display != param_display::normal ? 3 : 5;
  int mult = display != param_display::normal ? 100 : 1;
  stream << std::setprecision(prec) << plain.real() * (mult);
  if (unit.size()) stream << " " << unit;
  return stream.str();
}

bool 
param_topo::text_to_plain(
  std::string const& textual, plain_value& plain) const
{
  if (edit == param_edit::toggle)
  {
    if (textual == "Off") plain = plain_value::from_step(0);
    else if (textual == "On") plain = plain_value::from_step(1);
    else return false;
    return true;
  }

  if (type == param_type::name)
  {
    for (int i = 0; i < names.size(); i++)
      if (names[i] == textual)
      {
        plain = plain_value::from_step(i);
        return true;
      }
    return false;
  }

  if (type == param_type::item)
  {
    for (int i = 0; i < items.size(); i++)
      if (items[i].name == textual)
      {
        plain = plain_value::from_step(i);
        return true;
      }
    return false;
  }

  std::istringstream stream(textual);
  if (type == param_type::step)
  {
    int step = std::numeric_limits<int>::max();
    stream >> step;
    plain = plain_value::from_step(step);
    return min <= step && step <= max;
  }

  float real = std::numeric_limits<float>::max();
  stream >> real;
  real /= (display == param_display::normal) ? 1 : 100;
  plain = plain_value::from_real(real);
  return min <= real && real <= max;
}

}