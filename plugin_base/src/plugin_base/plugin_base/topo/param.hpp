#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/topo/shared.hpp>

#include <cmath>
#include <vector>
#include <string>

namespace plugin_base {

enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_format { plain, normalized };

enum class domain_display { normal, percentage };
enum class domain_type { toggle, step, name, item, linear, log };

// item in list
struct list_item final {
  int tag = -1;
  std::string id = {};
  std::string name = {};

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(list_item);
  template <class T> list_item(T const& item) : 
  tag(item.info.index), id(item.info.id), name(item.info.name) {}
  list_item(std::string const& id, std::string const& name, int tag) :
  tag(tag), id(id), name(name) {}
};

// parameter dsp
struct param_dsp final {
  param_rate rate;
  param_format format;
  param_direction direction;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_dsp);
};

// parameter ui
struct param_topo_gui final {
  gui_layout layout;
  gui_position position;
  gui_bindings bindings;
  gui_edit_type edit_type;
  gui_label_align label_align;
  gui_label_justify label_justify;
  gui_label_contents label_contents;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_topo_gui);
};

// parameter bounds
struct param_domain final {
  double min;
  double max;
  double exp;
  int precision;
  std::string unit;
  std::string default_;

  domain_type type;
  domain_display display;
  std::vector<list_item> items;
  std::vector<std::string> names;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_domain);
  bool is_real() const { return type == domain_type::log || type == domain_type::linear; }
};

// parameter in module
struct param_topo final {
  int section;
  param_dsp dsp;
  param_topo_gui gui;
  param_domain domain;
  component_info info;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_topo);

  // representation conversion
  plain_value raw_to_plain(double raw) const;
  double plain_to_raw(plain_value plain) const;
  normalized_value raw_to_normalized(double raw) const;
  double normalized_to_raw(normalized_value normalized) const;
  normalized_value plain_to_normalized(plain_value plain) const;
  plain_value normalized_to_plain(normalized_value normalized) const;

  // parse and format
  std::string plain_to_text(plain_value plain) const;
  std::string normalized_to_text(normalized_value normalized) const;
  bool text_to_plain(std::string const& textual, plain_value& plain) const;
  bool text_to_normalized(std::string const& textual, normalized_value& normalized) const;

  // parse default text
  plain_value default_plain() const;
  double default_raw() const { return plain_to_raw(default_plain()); }
  normalized_value default_normalized() const { return plain_to_normalized(default_plain()); }
};

inline normalized_value
param_topo::raw_to_normalized(double raw) const
{
  plain_value plain = raw_to_plain(raw);
  return plain_to_normalized(raw_to_plain(raw));
}

inline double
param_topo::normalized_to_raw(normalized_value normalized) const
{
  plain_value plain = normalized_to_plain(normalized);
  return plain_to_raw(plain);
}

inline double
param_topo::plain_to_raw(plain_value plain) const
{
  if (domain.is_real())
    return plain.real();
  else
    return plain.step();
}

inline plain_value 
param_topo::raw_to_plain(double raw) const
{
  assert(domain.min <= raw && raw <= domain.max);
  if(domain.is_real())
    return plain_value::from_real(raw);
  else
    return plain_value::from_step(raw);
}

inline normalized_value 
param_topo::plain_to_normalized(plain_value plain) const
{
  double range = domain.max - domain.min;
  if (!domain.is_real())
    return normalized_value((plain.step() - domain.min) / range);
  if (domain.type == domain_type::linear)
    return normalized_value((plain.real() - domain.min) / range);
  return normalized_value(std::pow((plain.real() - domain.min) * (1 / range), 1 / domain.exp));
}

inline plain_value 
param_topo::normalized_to_plain(normalized_value normalized) const
{
  double range = domain.max - domain.min;
  if (!domain.is_real())
    return plain_value::from_step(domain.min + std::floor(std::min(range, normalized.value() * (range + 1))));
  if (domain.type == domain_type::linear)
    return plain_value::from_real(domain.min + normalized.value() * range);
  return plain_value::from_real(std::pow(normalized.value(), domain.exp) * range + domain.min);
}

}
