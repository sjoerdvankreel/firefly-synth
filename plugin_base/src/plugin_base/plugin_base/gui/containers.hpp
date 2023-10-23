#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/components.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

// resizes single child on resize
class group_component :
public juce::GroupComponent {
public:
  void resized() override;
};

// grid component as opposed to grid layout
// resizes children on resize
class grid_component:
public juce::Component,
public autofit_component
{
  gui_dimension const _dimension;
  std::vector<gui_position> _positions = {};

public:
  void resized() override;
  int autofit_width() const override;
  int autofit_height() const override;

  void add(Component& child, gui_position const& position);
  void add(Component& child, bool vertical, int position) 
  { add(child, gui_position { vertical? position: 0, vertical? 0: position }); }

  // Can't intercept mouse as we may be invisible on top of 
  // another grid in case of param or section dependent visibility.
  grid_component(gui_dimension const& dimension) :
  _dimension(dimension) { setInterceptsMouseClicks(false, true); }
  grid_component(bool vertical, int count) :
  grid_component(gui_dimension { vertical ? count : 1, vertical ? 1 : count }) {}
};

// binding_component that hosts a single param_section_grid
class param_section_group :
public binding_component,
public group_component
{
public:
  param_section_group(plugin_gui* gui, module_desc const* module, param_section const* section):
  binding_component(gui, module, &section->gui.bindings, 0), group_component() { init(); }
};

// binding_component that hosts a number of plugin parameters
class param_section_grid :
public binding_component,
public grid_component
{
public:
  param_section_grid(plugin_gui* gui, module_desc const* module, param_section const* section):
  binding_component(gui, module, &section->gui.bindings, 0), grid_component(section->gui.dimension) { init(); }
};

}