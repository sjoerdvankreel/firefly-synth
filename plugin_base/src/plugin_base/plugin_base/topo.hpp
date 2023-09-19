#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <memory>

namespace plugin_base {

enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

enum class plugin_type { synth, fx };
enum class gui_layout { default_, horizontal, vertical, tabbed };

enum class param_dir { input, output };
enum class param_rate { accurate, block };
enum class param_label { none, name, value, both };
enum class param_display { normal, pct, pct_no_unit };
enum class param_type { step, name, item, linear, log };
enum class param_edit { toggle, list, text, knob, hslider, vslider };

class module_engine;
typedef std::unique_ptr<module_engine>(*
module_engine_factory)(int sample_rate, int max_frame_count);

// dimensions of own grid
struct gui_dimension final {
  int rows = -1;
  int columns = -1;
};

// position in parent grid
struct gui_position final {
  int row = -1;
  int column = -1;
  int row_span = 1;
  int column_span = 1;
};

// item in list
struct item_topo final {
  std::string id;
  std::string name;
  
  INF_DECLARE_MOVE_ONLY(item_topo);
  item_topo(std::string const& id, std::string const& name): 
  id(id), name(name) {}
};

// param gui section
struct section_topo final {
  int section;
  std::string name;
  gui_position position;
  gui_dimension dimension;

  INF_DECLARE_MOVE_ONLY(section_topo);
};

// param group in module
struct param_topo final {
  double min;
  double max;
  double exp;
  int section;
  int slot_count;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_;

  param_dir dir;
  param_type type;
  param_rate rate;
  param_edit edit;
  param_label label;
  param_display display;

  gui_layout layout;
  gui_position position;
  std::vector<item_topo> items;
  std::vector<std::string> names;

  INF_DECLARE_MOVE_ONLY(param_topo);
  bool is_real() const { return type == param_type::log || type == param_type::linear; }

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

// module group in plugin
struct module_topo final {
  int slot_count;
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
  INF_DECLARE_MOVE_ONLY(module_topo);
};

// plugin definition
struct plugin_topo final {
  int polyphony;
  std::string id;
  std::string name;
  plugin_type type;
  int version_major;
  int version_minor;

  int gui_min_width;
  int gui_default_width;
  float gui_aspect_ratio;
  gui_dimension dimension;
  std::vector<module_topo> modules;

  INF_DECLARE_MOVE_ONLY(plugin_topo);
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
  assert(min <= raw && raw <= max);
  if(is_real())
    return plain_value::from_real(raw);
  else
    return plain_value::from_step(raw);
}

inline normalized_value 
param_topo::plain_to_normalized(plain_value plain) const
{
  double range = max - min;
  if (!is_real())
    return normalized_value((plain.step() - min) / range);
  if (type == param_type::linear)
    return normalized_value((plain.real() - min) / range);
  return normalized_value(std::pow((plain.real() - min) * (1 / range), 1 / exp));
}

inline plain_value 
param_topo::normalized_to_plain(normalized_value normalized) const
{
  double range = max - min;
  if (!is_real())
    return plain_value::from_step(min + std::floor(std::min(range, normalized.value() * (range + 1))));
  if (type == param_type::linear)
    return plain_value::from_real(min + normalized.value() * range);
  return plain_value::from_real(std::pow(normalized.value(), exp) * range + min);
}

}
