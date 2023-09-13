#pragma once
#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <memory>

namespace plugin_base {

class module_engine;

enum class plugin_type { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_label { none, name, value, both };
// No_display maps times 100 but doesnt have the unit value. Useful for osc cents.
enum class param_percentage { off, on, no_unit };
enum class param_type { step, name, item, linear, log };
enum class param_edit { toggle, list, text, knob, hslider, vslider };

// item within list (persisted by guid)
struct item_topo final {
  std::string id;
  std::string name;
  item_topo(std::string const& id, std::string const& name): id(id), name(name) {}
  INF_DECLARE_MOVE_ONLY(item_topo);
};

// param within module
struct param_topo final {
  int group;
  double min;
  double max;
  double exp;
  param_type type;
  param_rate rate;
  param_edit edit;
  param_label label;
  param_direction direction;
  param_percentage percentage;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_text;
  std::vector<item_topo> items;
  std::vector<std::string> names;
  INF_DECLARE_MOVE_ONLY(param_topo);

  // Parses (!) text default -- dont call on the audio thread.
  double default_raw() const;
  plain_value default_plain() const;
  normalized_value default_normalized() const;

  // These are just casting union { int, float } (plain) <-> double (raw).
  plain_value raw_to_plain(double raw) const;
  double plain_to_raw(plain_value plain) const;

  // These perform actual conversion (log/linear/stepped scaling).
  normalized_value plain_to_normalized(plain_value plain) const;
  plain_value normalized_to_plain(normalized_value normalized) const;

  // Utility functions that combine the 2 steps above.
  normalized_value raw_to_normalized(double raw) const;
  double normalized_to_raw(normalized_value normalized) const;

  // Parse/format text.
  std::string plain_to_text(plain_value plain) const;
  std::string normalized_to_text(normalized_value normalized) const;
  bool text_to_plain(std::string const& textual, plain_value& plain) const;
  bool text_to_normalized(std::string const& textual, normalized_value& normalized) const;

  bool is_real() const { return type == param_type::log || type == param_type::linear; }
};

// grouping parameters for gui
struct param_group_topo final {
  int type;
  std::string name;
  param_group_topo(int type, std::string const& name): type(type), name(name) {}
  INF_DECLARE_MOVE_ONLY(param_group_topo);
};

// module group within plugin (may be more than 1 per group)
struct module_group_topo final {
  int module_count;
  std::string id;
  std::string name;
  module_scope scope;
  module_output output;
  std::vector<param_topo> params;
  std::vector<param_group_topo> param_groups;
  std::unique_ptr<module_engine>(*engine_factory)(int sample_rate, int max_frame_count);
  INF_DECLARE_MOVE_ONLY(module_group_topo);
};

// plugin definition
struct plugin_topo final {
  int polyphony;
  plugin_type type;
  int gui_default_width;
  float gui_aspect_ratio;
  std::vector<module_group_topo> module_groups;
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

// rather create some more copies than std::move() stuff around
// all topology is just move-only to prevent accidental copies
typedef plugin_topo(*plugin_topo_factory)(); 
typedef std::vector<item_topo>(*items_topo_factory)();

inline plain_value 
param_topo::raw_to_plain(double raw) const
{
  assert(min <= raw && raw <= max);
  if(is_real())
    return plain_value::from_real(raw);
  return plain_value::from_step(raw);
}

inline double 
param_topo::plain_to_raw(plain_value plain) const
{
  if (is_real())
    return plain.real();
  return plain.step();
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

}
