#pragma once

#include <plugin_base/shared/utility.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

namespace plugin_base {

struct module_topo;
struct gui_dimension;

// just a guess for validation, increase if needed
inline int constexpr topo_max = 1024;

enum class gui_label_justify { near, far, center };
enum class gui_label_align { top, bottom, left, right };
enum class gui_label_contents { none, name, value, both };
enum class gui_edit_type { toggle, list, autofit_list, text, knob, hslider, vslider };

typedef std::function<bool(std::vector<int> const& vs)>
gui_binding_selector;

// plugin and section metadata
struct topo_tag final {
  std::string id;
  std::string name;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(topo_tag);
};

// module and parameter metadata
struct topo_info final {
  int index;
  int slot_count;
  topo_tag tag;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(topo_info);
};

// label positioning
struct gui_label final {
  gui_label_align align;
  gui_label_justify justify;
  gui_label_contents contents;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_label);
};

// submenu with header and indices into main list
struct gui_submenu final {
  std::string name;
  std::vector<int> indices;
  std::vector<std::shared_ptr<gui_submenu>> children;
  
  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_submenu);
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
  gui_binding_selector selector = {};

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_binding);
  void validate(module_topo const& module, int slot_count) const;
  void bind(std::vector<int> const& params_, gui_binding_selector selector_);
};

// binding to enabled/visible
struct gui_bindings final {
  gui_binding enabled;
  gui_binding visible;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_bindings);
  void validate(module_topo const& module, int slot_count) const;
};

// dimensions of own grid (relative distribution)
// positive is relative, negative is absolute, 0 is auto
struct gui_dimension final {
  static inline int const auto_size = 0;
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

// color scheme
struct gui_colors final {
  juce::Colour tab_text = juce::Colour(0xFFFF8844);
  juce::Colour tab_button = juce::Colour(0xFF333333);
  juce::Colour tab_header = juce::Colour(0xFF222222);
  juce::Colour tab_background1 = juce::Colour(0xFF222222);
  juce::Colour tab_background2 = juce::Colour(0xFF111111);
  juce::Colour control_tick = juce::Colour(0xFFFF8844);
  juce::Colour control_text = juce::Colour(0xFFFFFFFF);
  juce::Colour control_outline = juce::Colour(0xFFAAAAAA);
  juce::Colour control_background = juce::Colour(0xFF111111);
  juce::Colour slider_thumb = juce::Colour(0xFFFF8844);
  juce::Colour slider_track1 = juce::Colour(0xFF222222);
  juce::Colour slider_track2 = juce::Colour(0xFF999999);
  juce::Colour slider_outline1 = juce::Colour(0xFF444444);
  juce::Colour slider_outline2 = juce::Colour(0xFFBBBBBB);
  juce::Colour slider_background = juce::Colour(0xFF000000);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_colors);
};

inline gui_dimension::
gui_dimension(int row_count, int column_count) : 
row_sizes(row_count, 1), column_sizes(column_count, 1) {}

inline gui_dimension::
gui_dimension(std::vector<int> const& row_sizes, std::vector<int> const& column_sizes) : 
row_sizes(row_sizes), column_sizes(column_sizes) {}

}
