#pragma once

#include <plugin_base/utility.hpp>

#include <vector>
#include <string>
#include <functional>

namespace plugin_base {

struct module_topo;
struct gui_dimension;

// just a guess for validation, increase if needed
inline int constexpr topo_max = 1024;

enum class gui_label_justify { near, center, far };
enum class gui_label_align { top, bottom, left, right };
enum class gui_label_contents { none, name, value, both };
enum class gui_layout { single, horizontal, vertical, tabbed };
enum class gui_edit_type { toggle, list, text, knob, hslider, vslider };

typedef bool(*
gui_binding_selector)(
  std::vector<int> const& values, 
  std::vector<int> const& context);

// plugin and section metadata
struct component_tag final {
  std::string id;
  std::string name;

  void validate() const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(component_tag);
};

// module and parameter metadata
struct component_info final {
  int index;
  int slot_count;
  component_tag tag;

  void validate() const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(component_info);
};

// position in parent grid
struct gui_position final {
  int row = -1;
  int column = -1;
  int row_span = 1;
  int column_span = 1;

  void validate(gui_dimension const& parent_dimension) const;
};

// binding to enabled/visible
struct gui_binding final {
  std::vector<int> params = {};
  std::vector<int> context = {};
  gui_binding_selector selector = {};

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(gui_binding);
  void validate(module_topo const& module, int slot_count) const;
};

// binding to enabled/visible
struct gui_bindings final {
  gui_binding enabled;
  gui_binding visible;

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(gui_bindings);
  void validate(module_topo const& module, int slot_count) const;
};

// dimensions of own grid (relative distribution)
struct gui_dimension final {
  std::vector<int> row_sizes = { 1 };
  std::vector<int> column_sizes = { 1 };

  void validate(
    std::vector<gui_position> const& children, 
    std::function<bool(int)> include, 
    std::function<bool(int)> always_visible) const;

  gui_dimension() = default;
  gui_dimension(gui_dimension const&) = default;
  gui_dimension(int row_count, int column_count);
  gui_dimension(std::vector<int> const& row_sizes, std::vector<int> const& column_sizes);
};

inline gui_dimension::
gui_dimension(int row_count, int column_count) : 
row_sizes(row_count, 1), column_sizes(column_count, 1) {}

inline gui_dimension::
gui_dimension(std::vector<int> const& row_sizes, std::vector<int> const& column_sizes) : 
row_sizes(row_sizes), column_sizes(column_sizes) {}

}
