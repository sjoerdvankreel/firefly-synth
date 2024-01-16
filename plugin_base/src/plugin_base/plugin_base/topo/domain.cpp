#include <plugin_base/topo/domain.hpp>

#include <set>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace plugin_base {

void
timesig::validate() const
{
  assert(den > 0);
  assert(num >= 0);
}

void
list_item::validate() const
{
  assert(id.size());
  assert(name.size());
}

std::string
timesig::to_text() const
{
  if (num == 0) return "0";
  return std::to_string(num) + "/" + std::to_string(den);
}

plain_value
param_domain::default_plain(int module_slot, int param_slot) const
{
  plain_value result;
  std::string default_ = default_selector(module_slot, param_slot);
  PB_ASSERT_EXEC(text_to_plain(false, default_, result));
  return result;
}

std::string
param_domain::raw_to_text(
  bool io, double raw) const
{
  plain_value plain(raw_to_plain(raw));
  return plain_to_text(io, plain);
}

std::string 
param_domain::normalized_to_text(
  bool io, normalized_value normalized) const
{
  plain_value plain(normalized_to_plain(normalized));
  return plain_to_text(io, plain);
}

bool 
param_domain::text_to_normalized(
  bool io, std::string const& textual, normalized_value& normalized) const
{
  plain_value plain;
  if(!text_to_plain(io, textual, plain)) return false;
  normalized = plain_to_normalized(plain);
  return true;
}

std::string
param_domain::plain_to_text(bool io, plain_value plain) const
{
  std::string prefix = "";
  if(min < 0 &&  ((is_real() && plain.real() >= 0) 
    || (!is_real() && plain.step() >= 0)))
    prefix = "+";

  switch (type)
  {
  case domain_type::name: return names[plain.step()];
  case domain_type::toggle: return plain.step() == 0 ? "Off" : "On";
  case domain_type::timesig: return timesigs[plain.step()].to_text();
  case domain_type::item: return io? items[plain.step()].id: items[plain.step()].name;
  case domain_type::step: return prefix + std::to_string(plain.step() + display_offset);
  default: break;
  }

  std::ostringstream stream;
  int mul = display == domain_display::percentage ? 100 : 1;
  stream << std::fixed << std::setprecision(precision) << (plain.real() * mul);
  if(unit.size()) stream << " " << unit;
  return prefix + stream.str();
}

bool 
param_domain::text_to_plain(
  bool io, std::string const& textual, plain_value& plain) const
{
  if (type == domain_type::timesig)
  {
    for (int i = 0; i < timesigs.size(); i++)
      if (textual == timesigs[i].to_text())
        return plain = plain_value::from_step(i), true;
    return false;
  }

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
      if ((io && items[i].id == textual) || (!io && items[i].name == textual))
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
    plain = plain_value::from_step(step - display_offset);
    return min <= plain.step() && plain.step() <= max;
  }

  float real = std::numeric_limits<float>::max();
  stream >> real;
  real /= (display == domain_display::normal) ? 1 : 100;
  plain = plain_value::from_real(real);
  return min <= plain.real() && plain.real() <= max;
}

void
param_domain::validate(int module_slot_count, int param_slot_count) const
{
  assert(max >= min);
  assert((type == domain_type::log) == (exp != 0));
  assert((type == domain_type::step) || (display_offset == 0));
  assert(display == domain_display::normal || type == domain_type::linear || type == domain_type::identity);

  if (type == domain_type::toggle || type == domain_type::identity)
  {
    assert(min == 0);
    assert(max == 1);
  }

  if (type == domain_type::name)
  {
    assert(min == 0);
    assert(unit.size() == 0);
    assert(names.size() > 0);
    assert(max == names.size() - 1);
    
    std::set<std::string> seen;
    for(int i = 0; i <names.size(); i++)
      PB_ASSERT_EXEC(seen.insert(names[i]).second);
  }

  if (type == domain_type::item)
  {
    assert(min == 0);
    assert(unit.size() == 0);
    assert(items.size() > 0);
    assert(max == items.size() - 1);

    std::set<std::string> seen_id;
    std::set<std::string> seen_name;
    for(int i = 0; i < items.size(); i++)
    {
      items[i].validate();
      PB_ASSERT_EXEC(seen_id.insert(items[i].id).second);
      PB_ASSERT_EXEC(seen_name.insert(items[i].name).second);
    }
  }

  if (type == domain_type::timesig)
  {
    assert(min == 0);
    assert(unit.size() == 0);
    assert(timesigs.size() > 0);
    assert(max == timesigs.size() - 1);

    std::set<std::string> seen;
    for (int i = 0; i < timesigs.size(); i++)
    {
      timesigs[i].validate();
      PB_ASSERT_EXEC(seen.insert(timesigs[i].to_text()).second);
    }
  }

  for(int mi = 0; mi < module_slot_count; mi++)
    for(int pi = 0; pi < param_slot_count; pi++)
      assert(default_selector(mi, pi).size());

  if (!is_real())
  {
    assert(precision == 0);
    assert((int)min == min);
    assert((int)max == max);
    assert(display == domain_display::normal);
    for(int mi = 0; mi < module_slot_count; mi++)
      for (int pi = 0; pi < param_slot_count; pi++)
      {
        assert(min <= default_plain(mi, pi).step());
        assert(max >= default_plain(mi, pi).step());
      }
  }

  if (is_real())
  {
    assert(0 <= precision && precision <= 10);
    for (int mi = 0; mi < module_slot_count; mi++)
      for (int pi = 0; pi < param_slot_count; pi++)
      {
        assert(min <= default_plain(mi, pi).real());
        assert(max >= default_plain(mi, pi).real());
      }
  }
}

}