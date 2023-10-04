#include <plugin_base/topo/param.hpp>

#include <sstream>
#include <iomanip>

namespace plugin_base {

plain_value
param_topo::default_plain() const
{
  plain_value result;
  INF_ASSERT_EXEC(text_to_plain(domain.default_, result));
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
  std::string prefix = "";
  if(domain.min < 0 && 
    ((domain.is_real() && plain.real() > 0) ||
    (!domain.is_real() && plain.step() > 0)))
    prefix = "+";

  if (domain.type == domain_type::toggle)
    return plain.step() == 0 ? "Off" : "On";
  switch (domain.type)
  {
  case domain_type::step:
  case domain_type::toggle:
    return prefix + std::to_string(plain.step());
  case domain_type::name: return domain.names[plain.step()];
  case domain_type::item: return domain.items[plain.step()].name;
  default: break;
  }

  std::ostringstream stream;
  int mul = domain.display == domain_display::percentage ? 100 : 1;
  stream << std::fixed << std::setprecision(domain.precision) << (plain.real() * mul);
  return prefix + stream.str();
}

bool 
param_topo::text_to_plain(
  std::string const& textual, plain_value& plain) const
{
  if (domain.type == domain_type::toggle)
  {
    if (textual == "Off") plain = plain_value::from_step(0);
    else if (textual == "On") plain = plain_value::from_step(1);
    else return false;
    return true;
  }

  if (domain.type == domain_type::name)
  {
    for (int i = 0; i < domain.names.size(); i++)
      if (domain.names[i] == textual)
      {
        plain = plain_value::from_step(i);
        return true;
      }
    return false;
  }

  if (domain.type == domain_type::item)
  {
    for (int i = 0; i < domain.items.size(); i++)
      if (domain.items[i].name == textual)
      {
        plain = plain_value::from_step(i);
        return true;
      }
    return false;
  }

  std::istringstream stream(textual);
  if (domain.type == domain_type::step)
  {
    int step = std::numeric_limits<int>::max();
    stream >> step;
    plain = plain_value::from_step(step);
    return domain.min <= step && step <= domain.max;
  }

  float real = std::numeric_limits<float>::max();
  stream >> real;
  real /= (domain.display == domain_display::normal) ? 1 : 100;
  plain = plain_value::from_real(real);
  return domain.min <= real && real <= domain.max;
}

}