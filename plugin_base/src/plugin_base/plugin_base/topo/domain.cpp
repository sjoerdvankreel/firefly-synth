#include <plugin_base/topo/domain.hpp>

#include <sstream>
#include <iomanip>

namespace plugin_base {

plain_value
param_domain::default_plain() const
{
  plain_value result;
  INF_ASSERT_EXEC(text_to_plain(default_, result));
  return result;
}

std::string 
param_domain::normalized_to_text(
  normalized_value normalized) const
{
  plain_value plain(normalized_to_plain(normalized));
  return plain_to_text(plain);
}

bool 
param_domain::text_to_normalized(
  std::string const& textual, normalized_value& normalized) const
{
  plain_value plain;
  if(!text_to_plain(textual, plain)) return false;
  normalized = plain_to_normalized(plain);
  return true;
}

std::string
param_domain::plain_to_text(plain_value plain) const
{
  std::string prefix = "";
  if(min < 0 &&  ((is_real() && plain.real() > 0) 
    || (!is_real() && plain.step() > 0)))
    prefix = "+";

  switch (type)
  {
  case domain_type::name: return names[plain.step()];
  case domain_type::item: return items[plain.step()].name;
  case domain_type::toggle: return plain.step() == 0 ? "Off" : "On";
  case domain_type::step: return prefix + std::to_string(plain.step());
  default: break;
  }

  std::ostringstream stream;
  int mul = display == domain_display::percentage ? 100 : 1;
  stream << std::fixed << std::setprecision(precision) << (plain.real() * mul);
  return prefix + stream.str();
}

bool 
param_domain::text_to_plain(
  std::string const& textual, plain_value& plain) const
{
  if (type == domain_type::name)
  {
    for (int i = 0; i < names.size(); i++)
      if (names[i] == textual)
        return plain = plain_value::from_step(i), true;
    return false;
  }

  if (type == domain_type::item)
  {
    for (int i = 0; i < items.size(); i++)
      if (items[i].name == textual)
        return plain = plain_value::from_step(i), true;
    return false;
  }

  if (type == domain_type::toggle)
  {
    if (textual == "On") return plain = plain_value::from_step(1), true;
    if (textual == "Off") return plain = plain_value::from_step(0), true;
    return false;
  }

  std::istringstream stream(textual);
  if (type == domain_type::step)
  {
    int step = std::numeric_limits<int>::max();
    stream >> step;
    plain = plain_value::from_step(step);
    return min <= step && step <= max;
  }

  float real = std::numeric_limits<float>::max();
  stream >> real;
  real /= (display == domain_display::normal) ? 1 : 100;
  plain = plain_value::from_real(real);
  return min <= real && real <= max;
}

}