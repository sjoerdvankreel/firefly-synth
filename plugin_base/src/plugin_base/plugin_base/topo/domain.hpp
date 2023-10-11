#pragma once

#include <plugin_base/dsp/value.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

namespace plugin_base {

enum class domain_display { normal, percentage };
enum class domain_type { toggle, step, name, item, linear, log };

// item in list
struct list_item final {
  int tag = -1;
  std::string id = {};
  std::string name = {};

  template <class T> static list_item 
  from_topo(T const* topo) { return list_item(topo->info); }

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(list_item);
  list_item(topo_info const& info);
  list_item(std::string const& id, std::string const& name, int tag);
};

// parameter bounds
struct param_domain final {
  double min;
  double max;
  double exp;
  int precision;
  int display_offset;
  std::string unit;
  std::string default_;

  domain_type type;
  domain_display display;
  std::vector<list_item> items;
  std::vector<std::string> names;  
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_domain);

  bool is_real() const;
  void validate() const;

  // parse default text - slow!
  double default_raw() const;
  plain_value default_plain() const;
  normalized_value default_normalized() const;

  // representation conversion
  plain_value raw_to_plain(double raw) const;
  double plain_to_raw(plain_value plain) const;
  normalized_value raw_to_normalized(double raw) const;
  double normalized_to_raw(normalized_value normalized) const;
  normalized_value plain_to_normalized(plain_value plain) const;
  plain_value normalized_to_plain(normalized_value normalized) const;

  // parse and format
  std::string raw_to_text(double raw) const;
  std::string plain_to_text(plain_value plain) const;
  std::string normalized_to_text(normalized_value normalized) const;
  bool text_to_plain(std::string const& textual, plain_value& plain) const;
  bool text_to_normalized(std::string const& textual, normalized_value& normalized) const;
};

inline list_item::
list_item(topo_info const& info):
tag(info.index), id(info.tag.id), name(info.tag.name) {}
inline list_item::
list_item(std::string const& id, std::string const& name, int tag) :
tag(tag), id(id), name(name) {}

inline double 
param_domain::default_raw() const 
{ return plain_to_raw(default_plain()); }
inline normalized_value 
param_domain::default_normalized() const 
{ return plain_to_normalized(default_plain()); }

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
    return normalized_value((plain.step() - min) / range);
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
