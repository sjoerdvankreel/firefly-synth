#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/utility.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

namespace plugin_base {

struct param_topo_mapping;
enum class domain_display { normal, percentage };
enum class domain_type { toggle, step, name, item, timesig, linear, log, identity };

typedef std::function<std::string(int module_slot, int param_slot)>
default_selector;

inline constexpr bool
domain_is_real(domain_type type)
{ return type == domain_type::log || type == domain_type::linear || type == domain_type::identity; }

// tempo relative to bpm
struct timesig
{
  int num;
  int den;

  void validate() const;
  std::string to_text() const;
};

// param topo mapping
struct param_topo_mapping final {
  int module_index;
  int module_slot;
  int param_index;
  int param_slot;

  bool operator==(param_topo_mapping const&) const = default;
  template <class T> auto& value_at(T& container) const 
  { return container[module_index][module_slot][param_index][param_slot]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_index][module_slot][param_index][param_slot]; }
};

// item in list
struct list_item final {
  std::string id = {};
  std::string name = {};
  // in case of auto binding
  param_topo_mapping param_topo = {};

  void validate() const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(list_item);
  list_item(topo_tag const& tag);
  list_item(std::string const& id, std::string const& name);
};

// parameter bounds
struct param_domain final {
  double min;
  double max;
  double exp;
  int precision;
  int display_offset;
  std::string unit;

  domain_type type;
  domain_display display;
  std::vector<list_item> items;
  std::vector<timesig> timesigs;
  std::vector<std::string> names;
  default_selector default_selector_;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_domain);

  bool is_real() const { return domain_is_real(type); }
  void validate(int module_slot_count, int param_slot_count) const;

  // representation conversion
  plain_value raw_to_plain(double raw) const;
  double plain_to_raw(plain_value plain) const;
  normalized_value raw_to_normalized(double raw) const;
  double normalized_to_raw(normalized_value normalized) const;
  normalized_value plain_to_normalized(plain_value plain) const;
  plain_value normalized_to_plain(normalized_value normalized) const;

  // parse default text - slow!
  double default_raw(int module_slot, int param_slot) const;
  plain_value default_plain(int module_slot, int param_slot) const;
  normalized_value default_normalized(int module_slot, int param_slot) const;

  // parse and format
  std::string raw_to_text(bool io, double raw) const;
  std::string plain_to_text(bool io, plain_value plain) const;
  std::string normalized_to_text(bool io, normalized_value normalized) const;
  bool text_to_plain(bool io, std::string const& textual, plain_value& plain) const;
  bool text_to_normalized(bool io, std::string const& textual, normalized_value& normalized) const;

  // for time critical stuff, caller has to specify domain
  template <domain_type DomainType>
  double plain_to_raw_fast(plain_value plain) const;
  template <domain_type DomainType>
  double normalized_to_raw_fast(normalized_value normalized) const;
  template <domain_type DomainType>
  plain_value normalized_to_plain_fast(normalized_value normalized) const;
};

inline list_item::
list_item(topo_tag const& tag):
id(tag.id), name(tag.name) {}
inline list_item::
list_item(std::string const& id, std::string const& name) :
id(id), name(name) {}

inline double 
param_domain::default_raw(int module_slot, int param_slot) const
{ return plain_to_raw(default_plain(module_slot, param_slot)); }
inline normalized_value 
param_domain::default_normalized(int module_slot, int param_slot) const
{ return plain_to_normalized(default_plain(module_slot, param_slot)); }

inline normalized_value
param_domain::raw_to_normalized(double raw) const
{ return plain_to_normalized(raw_to_plain(raw)); }
inline double
param_domain::normalized_to_raw(normalized_value normalized) const
{ return plain_to_raw(normalized_to_plain(normalized)); }

inline double
param_domain::plain_to_raw(plain_value plain) const
{ return is_real()? plain.real(): plain.step(); }

inline plain_value 
param_domain::raw_to_plain(double raw) const
{
  assert(min <= raw && raw <= max);
  return is_real()? plain_value::from_real(raw): plain_value::from_step(raw);
}

inline normalized_value 
param_domain::plain_to_normalized(plain_value plain) const
{
  double range = max - min;
  if (!is_real())
    return normalized_value((plain.step() - min) / (range == 0? 1: range));
  if(type == domain_type::identity)
    return normalized_value(plain.real());
  if (type == domain_type::linear)
    return normalized_value((plain.real() - min) / range);

  // correct for rounding errors
  assert(type == domain_type::log);
  double plain_real = std::clamp((double)plain.real(), min, max);
  return normalized_value(std::pow((plain_real - min) * (1 / range), 1 / exp));
}

inline plain_value 
param_domain::normalized_to_plain(normalized_value normalized) const
{
  switch (type)
  {
  case domain_type::log: return normalized_to_plain_fast<domain_type::log>(normalized);
  case domain_type::step: return normalized_to_plain_fast<domain_type::step>(normalized);
  case domain_type::name: return normalized_to_plain_fast<domain_type::name>(normalized);
  case domain_type::item: return normalized_to_plain_fast<domain_type::item>(normalized);
  case domain_type::linear: return normalized_to_plain_fast<domain_type::linear>(normalized);
  case domain_type::toggle: return normalized_to_plain_fast<domain_type::toggle>(normalized);
  case domain_type::timesig: return normalized_to_plain_fast<domain_type::timesig>(normalized);
  case domain_type::identity: return normalized_to_plain_fast<domain_type::identity>(normalized);
  default: assert(false); return {};
  }
}

// dsp versions

template <domain_type DomainType> inline double
param_domain::normalized_to_raw_fast(normalized_value normalized) const
{
  assert(type == DomainType);
  auto plain = normalized_to_plain_fast<DomainType>(normalized);
  return plain_to_raw_fast<DomainType>(plain);
}

template <domain_type DomainType> inline double
param_domain::plain_to_raw_fast(plain_value plain) const
{
  assert(type == DomainType);
  if constexpr(domain_is_real(DomainType)) return plain.real();
  else return plain.step();
}

template <domain_type DomainType> inline plain_value
param_domain::normalized_to_plain_fast(normalized_value normalized) const
{
  double range = max - min;
  if constexpr (!domain_is_real(DomainType))
    return plain_value::from_step(min + std::floor(std::min(range, normalized.value() * (range + 1))));
  else if constexpr(DomainType == domain_type::identity)
    return plain_value::from_real(normalized.value());
  else if constexpr(DomainType == domain_type::linear)
    return plain_value::from_real(min + normalized.value() * range);
  else if constexpr(DomainType == domain_type::log)
  {
    double normalized_real = std::clamp((double)normalized.value(), 0.0, 1.0);
    return plain_value::from_real(std::pow(normalized_real, exp) * range + min);
  }
  else
    static_assert(dependent_always_false_v<0>);
}

}
