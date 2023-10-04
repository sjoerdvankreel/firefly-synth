#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <memory>

namespace plugin_base {

enum class plugin_type { synth, fx };
enum class gui_layout { single, horizontal, vertical, tabbed };

enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

enum class domain_display { normal, percentage };
enum class domain_type { step, name, item, linear, log };

enum class param_dir { input, output };
enum class param_rate { accurate, block };
enum class param_format { plain, normalized };
enum class param_edit { toggle, list, text, knob, hslider, vslider };
enum class param_label_justify { near, center, far };
enum class param_label_align { top, bottom, left, right };
enum class param_label_contents { none, name, value, both };

class module_engine;
typedef std::unique_ptr<module_engine>(*
module_engine_factory)(int slot, int sample_rate, int max_frame_count);
typedef bool(*
gui_binding_selector)(std::vector<int> const& values, std::vector<int> const& context);

// position in parent grid
struct gui_position final {
  int row = -1;
  int column = -1;
  int row_span = 1;
  int column_span = 1;
};

// dimensions of own grid (relative distribution)
struct gui_dimension final {
  std::vector<int> row_sizes = { 1 };
  std::vector<int> column_sizes = { 1 };

  gui_dimension() = default;
  gui_dimension(gui_dimension const&) = default;
  gui_dimension(int row_count, int column_count):
  row_sizes(row_count, 1), column_sizes(column_count, 1) {}
  gui_dimension(std::vector<int> const& row_sizes, std::vector<int> const& column_sizes):
  row_sizes(row_sizes), column_sizes(column_sizes) {}
};

// binding to ui enabled/visible
struct gui_bindings final {
  std::vector<int> enabled_params = {};
  std::vector<int> enabled_context = {};
  gui_binding_selector enabled_selector = {};
  std::vector<int> visibility_params = {};
  std::vector<int> visibility_context = {};
  gui_binding_selector visibility_selector = {};
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(gui_bindings);
};

// item in list
struct item_topo final {
  int tag = -1;
  std::string id = {};
  std::string name = {};
  
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(item_topo);
  // copy from param or module
  template <class T> item_topo(T const& item) : tag(item.index), id(item.id), name(item.name) {}
  item_topo(std::string const& id, std::string const& name, int tag): tag(tag), id(id), name(name) {}
};

// param gui section
struct section_topo final {
  int index;
  std::string name;
  gui_position position;
  gui_bindings bindings;
  gui_dimension dimension;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(section_topo);
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
  std::vector<item_topo> items;
  std::vector<std::string> names;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_domain);
};

// param group in module
struct param_topo final {
  int index;
  int section;
  int slot_count;
  std::string id;
  std::string name;
  param_domain domain;

  param_dir dir;
  param_rate rate;
  param_edit edit;
  param_format format;
  param_label_align label_align;
  param_label_justify label_justify;
  param_label_contents label_contents;

  gui_layout layout;
  gui_position position;
  gui_bindings bindings;

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

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_topo);
  bool is_real() const { return domain.type == domain_type::log || domain.type == domain_type::linear; }
};

// module group in plugin
struct module_topo final {
  int index;
  int slot_count;
  int output_count;
  std::string id;
  std::string name;
  module_stage stage;
  module_output output;

  gui_layout layout;
  gui_position position;
  gui_dimension dimension;
  std::vector<param_topo> params;
  std::vector<section_topo> sections;

  module_engine_factory engine_factory;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_topo);
};

// plugin definition
struct plugin_topo final {
  int polyphony;
  std::string id;
  std::string name;
  plugin_type type;
  int version_major;
  int version_minor;
  std::string preset_extension;

  int gui_min_width;
  int gui_max_width;
  int gui_default_width;
  int gui_aspect_ratio_width;
  int gui_aspect_ratio_height;
  gui_dimension dimension;
  std::vector<module_topo> modules;

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_topo);
};

inline normalized_value
param_topo::raw_to_normalized(double raw) const
{
  plain_value plain = raw_to_plain(raw);
  return plain_to_normalized(plain);
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
  if (is_real())
    return plain.real();
  else
    return plain.step();
}

inline plain_value 
param_topo::raw_to_plain(double raw) const
{
  assert(domain.min <= raw && raw <= domain.max);
  if(is_real())
    return plain_value::from_real(raw);
  else
    return plain_value::from_step(raw);
}

inline normalized_value 
param_topo::plain_to_normalized(plain_value plain) const
{
  double range = domain.max - domain.min;
  if (!is_real())
    return normalized_value((plain.step() - domain.min) / range);
  if (domain.type == domain_type::linear)
    return normalized_value((plain.real() - domain.min) / range);
  return normalized_value(std::pow((plain.real() - domain.min) * (1 / range), 1 / domain.exp));
}

inline plain_value 
param_topo::normalized_to_plain(normalized_value normalized) const
{
  double range = domain.max - domain.min;
  if (!is_real())
    return plain_value::from_step(domain.min + std::floor(std::min(range, normalized.value() * (range + 1))));
  if (domain.type == domain_type::linear)
    return plain_value::from_real(domain.min + normalized.value() * range);
  return plain_value::from_real(std::pow(normalized.value(), domain.exp) * range + domain.min);
}

}
