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
enum class domain_type { toggle, step, name, item, timesig, linear, log };

typedef std::function<std::string(int module_slot, int param_slot)>
default_selector;

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

  template <class T> auto& value_at(T& container) const 
  { return container[module_index][module_slot][param_index][param_slot]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_index][module_slot][param_index][param_slot]; }
};

// item in list
struct list_item final {
  std::string id = {};
  std::string name = {};

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(list_item);
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
  default_selector default_selector;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_domain);

  bool is_real() const;
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
inline bool 
param_domain::is_real() const 
{ return type == domain_type::log || type == domain_type::linear; }

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
  if (type == domain_type::linear)
    return normalized_value((plain.real() - min) / range);
  return normalized_value(std::pow((plain.real() - min) * (1 / range), 1 / exp));
}

inline plain_value 
param_domain::normalized_to_plain(normalized_value normalized) const
{
  double range = max - min;
  if (!is_real())
    return plain_value::from_step(min + std::floor(std::min(range, normalized.value() * (range + 1))));
  if (type == domain_type::linear)
    return plain_value::from_real(min + normalized.value() * range);
  return plain_value::from_real(std::pow(normalized.value(), exp) * range + min);
}

}
