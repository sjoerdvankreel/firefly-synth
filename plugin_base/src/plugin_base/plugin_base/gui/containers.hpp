#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/components.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

// needs child to be able to autofit in 1 direction
class autofit_viewport:
public juce::Viewport
{
  lnf* const _lnf;
public:
  void resized() override;
  autofit_viewport(lnf* lnf) : _lnf(lnf) {}
};

// adds some margin around another component
class margin_component :
public juce::Component,
public autofit_component
{
  juce::Component* const _child;
  juce::BorderSize<int> const _margin;
public:
  void resized() override;
  int fixed_width(int parent_w, int parent_h) const override;
  int fixed_height(int parent_w, int parent_h) const override;
  margin_component(juce::Component* child, juce::BorderSize<int> const& margin):
  _child(child), _margin(margin) { add_and_make_visible(*this, *child); }
};

// d&d support for the entire plugin UI
class plugin_drag_drop_container :
public juce::Component,
public juce::DragAndDropContainer,
public autofit_component
{
  juce::Component* const _child;
public:
  void resized() override 
  { getChildComponent(0)->setBounds(getLocalBounds()); }
  plugin_drag_drop_container(juce::Component* child) : _child(child) 
  { add_and_make_visible(*this, *child); }

  int fixed_width(int parent_w, int parent_h) const override 
  { return dynamic_cast<autofit_component*>(getChildComponent(0))->fixed_width(parent_w, parent_h); }
  int fixed_height(int parent_w, int parent_h) const override 
  { return dynamic_cast<autofit_component*>(getChildComponent(0))->fixed_height(parent_w, parent_h); }
};

// displays a child component based on extra state changes
class extra_state_container:
public juce::Component,
public extra_state_listener
{
  plugin_gui* const _gui = {};
  std::string const _state_key = {};
  std::unique_ptr<juce::Component> _child = {};
protected:
  plugin_gui* const gui() { return _gui; }
  std::string const& state_key() const { return _state_key; }
  virtual std::unique_ptr<juce::Component> create_child() = 0;
public:
  void extra_state_changed() override;
  extra_state_container(plugin_gui* gui, std::string const& state_key);
  void resized() override { if (_child) { _child->setBounds(getLocalBounds()); } }
  virtual ~extra_state_container() { _gui->extra_state_()->remove_listener(_state_key, this); }
};

// displays a child component based on module tab changes
class tabbed_module_section_container:
public extra_state_container
{
private:
  int const _section_index;
  std::function<std::unique_ptr<juce::Component>(int module_index)> _factory;
protected:
  std::unique_ptr<juce::Component> create_child() override;
public:
  tabbed_module_section_container(
    plugin_gui* gui, int section_index,
    std::function<std::unique_ptr<juce::Component>(int module_index)> factory);
};

// tab bar button with drag-hover support
class tab_bar_button :
public juce::TabBarButton,
public juce::DragAndDropTarget
{
  bool const _select_tab_on_drag_hover;
  module_desc const* const _drag_module_descriptors;
public:
  tab_bar_button(juce::String const& name, juce::TabbedButtonBar& owner, bool select_tab_on_drag_hover, module_desc const* drag_module_descriptors) :
  TabBarButton(name, owner), _select_tab_on_drag_hover(select_tab_on_drag_hover), _drag_module_descriptors(drag_module_descriptors) {}

  juce::MouseCursor getMouseCursor() override;
  void mouseDrag(juce::MouseEvent const& e) override;
  void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& details) override;
  void itemDropped(juce::DragAndDropTarget::SourceDetails const& details) override { }
  bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& details) override { return _select_tab_on_drag_hover; }
};

// tab component with persistent selection and change listener
class tab_component :
public juce::TabbedComponent,
public extra_state_listener
{
  extra_state* const _state;
  std::string const _storage_id;
  bool const _select_tab_on_drag_hover;
  module_desc const* const _drag_module_descriptors;

protected:
  juce::TabBarButton* createTabButton(juce::String const& name, int index) override 
  { return new tab_bar_button(name, *tabs, _select_tab_on_drag_hover, _drag_module_descriptors); }

public:
  std::function<void(int)> tab_changed;
  ~tab_component();
  tab_component(
    extra_state* state, std::string const& storage_id, juce::TabbedButtonBar::Orientation orientation,
    bool select_tab_on_drag_hover, module_desc const* drag_module_descriptors);

  void extra_state_changed() override
  { setCurrentTabIndex(std::clamp(_state->get_int(_storage_id, 0), 0, getNumTabs())); }
  void currentTabChanged(int index, juce::String const& name) override
  { if (tab_changed != nullptr) tab_changed(index); }
};

// rounded rectangle container
enum class rounded_container_mode { fill, stroke, both };
class rounded_container:
public juce::Component,
public autofit_component
{
  int const _radius;
  int const _vpadding;
  int const _margin_left;
  bool const _vertical;
  juce::Component* _child;
  juce::Colour const _background;
  juce::Colour const _outline;
  rounded_container_mode const _mode;

  int radius_and_padding() const { return _radius + _vpadding; }

public:
  void resized() override;
  void paint(juce::Graphics& g) override;
  int fixed_width(int parent_w, int parent_h) const override;
  int fixed_height(int parent_w, int parent_h) const override;

  rounded_container(
    juce::Component* child, int radius, int vpadding, int margin_left, bool vertical, 
    rounded_container_mode mode, juce::Colour const& background, juce::Colour const& outline):
  _radius(radius), _vpadding(vpadding), _margin_left(margin_left), 
  _vertical(vertical), _background(background), _outline(outline), _mode(mode)
  { add_and_make_visible(*this, *child); }
};

// hosts a number of section params
class param_section_container:
public binding_component,
public rounded_container
{
public:
  param_section_container(plugin_gui* gui, lnf* lnf, module_desc const* module, param_section const* section, juce::Component* child, int margin_left);
};

// grid component as opposed to grid layout
// resizes children on resize
class grid_component:
public juce::Component,
public autofit_component
{
  float const _vgap_size;
  float const _hgap_size;
  int const _autofit_row;
  int const _autofit_column;
  gui_dimension const _dimension;
  std::vector<gui_position> _positions = {};

public:
  void resized() override;
  int fixed_width(int parent_w, int parent_h) const override;
  int fixed_height(int parent_w, int parent_h) const override;

  void add(Component& child, gui_position const& position);
  void add(Component& child, bool vertical, int position) 
  { add(child, gui_position { vertical? position: 0, vertical? 0: position }); }

  // Can't intercept mouse as we may be invisible on top of 
  // another grid in case of param or section dependent visibility.
  grid_component(gui_dimension const& dimension, float vgap_size, float hgap_size, int autofit_row, int autofit_column) :
  _vgap_size(vgap_size), _hgap_size(hgap_size), _dimension(dimension), _autofit_row(autofit_row), _autofit_column(autofit_column)
  { setInterceptsMouseClicks(false, true); }
  grid_component(bool vertical, int count, float vgap_size, float hgap_size, int autofit_row, int autofit_column) :
  grid_component(gui_dimension { vertical ? count : 1, vertical ? 1 : count }, vgap_size, hgap_size, autofit_row, autofit_column) {}
};

}