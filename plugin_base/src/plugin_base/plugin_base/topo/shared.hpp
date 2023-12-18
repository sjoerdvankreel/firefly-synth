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

enum class gui_scroll_mode { none, vertical };
enum class gui_label_justify { near, far, center };
enum class gui_label_align { top, bottom, left, right };
enum class gui_label_contents { none, name, short_name, value, both };
enum class gui_edit_type { none, output, toggle, list, autofit_list, knob, hslider, vslider };

typedef std::function<bool(int module_slot)>
gui_slot_binding_selector;

typedef std::function<bool(std::vector<int> const& vs)>
gui_param_binding_selector;

// plugin and section metadata
struct topo_tag final {
  std::string id;
  std::string name;
  std::string short_name;
  bool name_one_based = true;

  void validate() const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(topo_tag);
};

// module and parameter metadata
struct topo_info final {
  int index;
  int slot_count;
  topo_tag tag;

  void validate() const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(topo_info);
};

// label positioning
struct gui_label final {
  gui_label_align align;
  gui_label_justify justify;
  gui_label_contents contents;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_label);
};

// submenu with header and indices into main list
struct gui_submenu final {
  std::string name;
  bool is_subheader;
  std::vector<int> indices;
  std::vector<std::shared_ptr<gui_submenu>> children;
  
  void validate() const;
  void add_subheader(std::string const& name);
  std::shared_ptr<gui_submenu> add_submenu(std::string const& name);
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_submenu);
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
  gui_slot_binding_selector slot_selector = {};
  gui_param_binding_selector param_selector = {};

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_binding);
  void validate(module_topo const& module, int slot_count) const;
  
  void bind_slot(gui_slot_binding_selector selector_);
  void bind_params(std::vector<int> const& params_, gui_param_binding_selector selector_);
  bool is_bound() const { return slot_selector != nullptr || param_selector != nullptr; }
};

// binding to enabled/visible
struct gui_bindings final {
  gui_binding enabled;
  gui_binding visible;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_bindings);
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
  juce::Colour tab_text_inactive = juce::Colour(0xFFAAAAAA);
  juce::Colour tab_button = juce::Colour(0xFF333333);
  juce::Colour tab_header = juce::Colour(0xFF222222);
  juce::Colour tab_background1 = juce::Colour(0xFF222222);
  juce::Colour tab_background2 = juce::Colour(0xFF111111);
  juce::Colour graph_grid = juce::Colour(0xFFFFFFFF);
  juce::Colour graph_background = juce::Colour(0xFF000000);
  juce::Colour graph_foreground = juce::Colour(0xFFFF8844);
  juce::Colour bubble_outline = juce::Colour(0xFFFF8844);
  juce::Colour knob_thumb = juce::Colour(0xFFFF8844);
  juce::Colour knob_track1 = juce::Colour(0xFF222222);
  juce::Colour knob_track2 = juce::Colour(0xFFFF8844);
  juce::Colour knob_background1 = juce::Colour(0xFF222222);
  juce::Colour knob_background2 = juce::Colour(0xFF999999);
  juce::Colour section_outline1 = juce::Colour(0xFF884422);
  juce::Colour section_outline2 = juce::Colour(0xFF444444);
  juce::Colour slider_thumb = juce::Colour(0xFFFF8844);
  juce::Colour slider_track1 = juce::Colour(0xFF222222);
  juce::Colour slider_track2 = juce::Colour(0xFF999999);
  juce::Colour slider_outline1 = juce::Colour(0xFF444444);
  juce::Colour slider_outline2 = juce::Colour(0xFFBBBBBB);
  juce::Colour slider_background = juce::Colour(0xFF000000);
  juce::Colour edit_text = juce::Colour(0xFFFFFFFF);
  juce::Colour table_header = juce::Colour(0xFFFF8844);
  juce::Colour label_text = juce::Colour(0xFFEEEEEE);
  juce::Colour control_tick = juce::Colour(0xFFFF8844);
  juce::Colour control_text = juce::Colour(0xFFEEEEEE);
  juce::Colour control_outline = juce::Colour(0xFFAAAAAA);
  juce::Colour control_background = juce::Colour(0xFF111111);
  juce::Colour custom_background1 = juce::Colour(0xFF222222);
  juce::Colour custom_background2 = juce::Colour(0xFF111111);
  juce::Colour scrollbar_thumb = juce::Colour(0xFFFF8844);
  juce::Colour scrollbar_background = juce::Colour(0xFF444444);
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(gui_colors);
};

inline gui_dimension::
gui_dimension(int row_count, int column_count) : 
row_sizes(row_count, 1), column_sizes(column_count, 1) {}

inline gui_dimension::
gui_dimension(std::vector<int> const& row_sizes, std::vector<int> const& column_sizes) : 
row_sizes(row_sizes), column_sizes(column_sizes) {}

}
