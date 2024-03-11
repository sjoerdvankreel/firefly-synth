#include <plugin_base/gui/containers.hpp>

using namespace juce;

namespace plugin_base {

tab_component::
~tab_component()
{ _state->remove_listener(_storage_id, this); }

tab_component::
tab_component(extra_state* state, std::string const& storage_id, juce::TabbedButtonBar::Orientation orientation) :
juce::TabbedComponent(orientation), _state(state), _storage_id(storage_id)
{ state->add_listener(storage_id, this); }

void 
autofit_viewport::resized()
{
  auto& fit = dynamic_cast<autofit_component&>(*getViewedComponent());
  getViewedComponent()->setSize(getWidth() - _lnf->getDefaultScrollbarWidth() - 2, fit.fixed_height(getWidth(), getHeight()));
}

extra_state_container::
extra_state_container(plugin_gui* gui, std::string const& state_key):
_gui(gui), _state_key(state_key)
{ _gui->extra_state_()->add_listener(state_key, this); }

void 
extra_state_container::extra_state_changed()
{
  if(_child) removeChildComponent(_child.get());
  _child = create_child();
  _child->setBounds(getLocalBounds());
  add_and_make_visible(*this, *_child.get());
}

param_section_container::
param_section_container(plugin_gui* gui, lnf* lnf, module_desc const* module, param_section const* section, juce::Component* child) :
  binding_component(gui, module, &section->gui.bindings, 0),
  rounded_container(child, 
    lnf->theme_settings().section_corner_radius, false, rounded_container_mode::both,
    lnf->module_gui_colors(module->module->info.tag.full_name).section_outline1, 
    lnf->module_gui_colors(module->module->info.tag.full_name).section_outline2) {
  init(); 
}

tabbed_module_section_container::
tabbed_module_section_container(plugin_gui* gui, int section_index,
  std::function<std::unique_ptr<juce::Component>(int module_index)> factory):
extra_state_container(gui, module_section_tab_key(*gui->gui_state()->desc().plugin, section_index)),
_section_index(section_index), _factory(factory) {}

std::unique_ptr<Component> 
tabbed_module_section_container::create_child()
{
  int tab_index = gui()->extra_state_()->get_num(state_key(), 0);
  auto const& tab_order = gui()->gui_state()->desc().plugin->gui.module_sections[_section_index].tab_order;
  tab_index = std::clamp(tab_index, 0, (int)tab_order.size() - 1);
  return _factory(tab_order[tab_index]);
}

void 
margin_component::resized()
{
  Rectangle<int> bounds(getLocalBounds());
  Rectangle<int> child_bounds(
    bounds.getX() + _margin.getLeft(), 
    bounds.getY() + _margin.getTop(), 
    bounds.getWidth() - _margin.getLeftAndRight(), 
    bounds.getHeight() - _margin.getTopAndBottom());
  assert(getNumChildComponents() == 1);
  getChildComponent(0)->setBounds(child_bounds);
}

int 
rounded_container::fixed_width(int parent_w, int parent_h) const
{
  auto child = getChildComponent(0);
  auto& fit = dynamic_cast<autofit_component&>(*child);
  assert(fit.fixed_width(parent_w - _radius, parent_h - _radius) > 0);
  return fit.fixed_width(parent_w - _radius, parent_h - _radius) + _radius;
}

int 
rounded_container::fixed_height(int parent_w, int parent_h) const
{
  auto child = getChildComponent(0);
  auto& fit = dynamic_cast<autofit_component&>(*child);
  assert(fit.fixed_height(parent_w - _radius, parent_h - _radius) > 0);
  return fit.fixed_height(parent_w - _radius, parent_h - _radius) + _radius;
}

void
rounded_container::resized()
{
  Rectangle<int> bounds(getLocalBounds());
  Rectangle<int> child_bounds(
    bounds.getX() + _radius / 2,
    bounds.getY() + _radius / 2,
    bounds.getWidth() - _radius,
    bounds.getHeight() - _radius);
  assert(getNumChildComponents() == 1);
  getChildComponent(0)->setBounds(child_bounds);
}

void
rounded_container::paint(Graphics& g)
{
  if (_mode == rounded_container_mode::both)
  {
    if (!_vertical) g.setGradientFill(ColourGradient(
      _color1.darker(1.75), 0, 0, _color2.darker(1.75), 0, getHeight(), false));
    else g.setGradientFill(ColourGradient(
      _color2.darker(1.75), 0, 0, _color1.darker(1.75), getWidth(), 0, false));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), _radius);
    if (_vertical) g.setGradientFill(ColourGradient(
      _color1, 0, 0, _color2, 0, getHeight(), false));
    else g.setGradientFill(ColourGradient(
      _color2, 0, 0, _color1, getWidth(), 0, false));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), _radius, 1);
    return;
  }

  if(_vertical)
    g.setGradientFill(juce::ColourGradient(
      _color1, 0, 0, _color2, 0, getHeight(), false));
  else
    g.setGradientFill(juce::ColourGradient(
      _color2, 0, 0, _color1, getWidth(), 0, false));

  if(_mode == rounded_container_mode::fill)
    g.fillRoundedRectangle(getLocalBounds().toFloat(), _radius);
  else if(_mode == rounded_container_mode::stroke)
    g.drawRoundedRectangle(getLocalBounds().toFloat(), _radius, 1);
  else
    assert(false);
}

void
grid_component::add(Component& child, gui_position const& position)
{
  assert(position.row >= 0);
  assert(position.column >= 0);
  assert(position.row_span > 0);
  assert(position.column_span > 0);
  assert(position.row + position.row_span <= _dimension.row_sizes.size());
  assert(position.column + position.column_span <= _dimension.column_sizes.size());
  add_and_make_visible(*this, child);
  _positions.push_back(position);
}

int 
grid_component::fixed_width(int parent_w, int parent_h) const
{
  // ignore span
  int result = 0;
  for(int c = 0; c < _dimension.column_sizes.size(); c++)
    for (int i = 0; i < _positions.size(); i++)
      if(_positions[i].column == c)
        if(_positions[i].row == _autofit_row)
        {
          auto child_ptr = getChildComponent(i);
          assert(dynamic_cast<autofit_component*>(child_ptr));
          auto& child = dynamic_cast<autofit_component&>(*child_ptr);
          assert(child.fixed_width(parent_w, parent_h) > 0);
          result += child.fixed_width(parent_w, parent_h);
        }
  result += (_dimension.column_sizes.size() - 1) * _gap_size;
  // correct for rounding errors
  return result + (int)std::ceil(_dimension.column_sizes.size() * 0.5f);
}

int 
grid_component::fixed_height(int parent_w, int parent_h) const
{
  // ignore span
  int result = 0;
  for (int r = 0; r < _dimension.row_sizes.size(); r++)
    for (int i = 0; i < _positions.size(); i++)
      if (_positions[i].row == r)
        if (_positions[i].column == _autofit_column)
        {
          auto& child = dynamic_cast<autofit_component&>(*getChildComponent(i));
          assert(child.fixed_height(parent_w, parent_h) > 0);
          result += child.fixed_height(parent_w, parent_h);
        }
  result += (_dimension.row_sizes.size() - 1) * _gap_size + _dimension.row_sizes.size();
  // correct for rounding errors
  return result + (int)std::ceil(_dimension.row_sizes.size() * 0.5f);
}

void 
grid_component::resized()
{
  Grid grid;
  grid.rowGap = Grid::Px(_gap_size);
  grid.columnGap = Grid::Px(_gap_size);

  // Note: this doesnt take into account autosizing interaction (i.e. multiple rows or cols being autosize target).
  float row_height_even_distrib = (getHeight() - _gap_size * (_dimension.row_sizes.size() - 1)) / (float)_dimension.row_sizes.size();
  float col_width_even_distrib = (getWidth() - _gap_size * (_dimension.column_sizes.size() - 1)) / (float)_dimension.column_sizes.size();

  for(int i = 0; i < _dimension.row_sizes.size(); i++)
    if(_dimension.row_sizes[i] > 0)
      grid.templateRows.add(Grid::Fr(_dimension.row_sizes[i]));
    else if (_dimension.row_sizes[i] < 0)
      grid.templateRows.add(Grid::Px(-_dimension.row_sizes[i]));
    else 
    {
      // autosize, dont bother with span
      int max_col_height = 0;
      for(int p = 0; p < _positions.size(); p++)
        if(_positions[p].column == _autofit_column && _positions[p].row == i)
        {
          auto autofit_child = dynamic_cast<autofit_component*>(getChildComponent(p));
          assert(autofit_child);
          auto fixed_height = autofit_child->fixed_height(col_width_even_distrib, row_height_even_distrib);
          assert(fixed_height > 0);
          max_col_height = std::max(max_col_height, fixed_height);
        }
      grid.templateRows.add(Grid::Px(max_col_height));
    }

  for(int i = 0; i < _dimension.column_sizes.size(); i++)
    if(_dimension.column_sizes[i] > 0)
      grid.templateColumns.add(Grid::Fr(_dimension.column_sizes[i]));
    else if (_dimension.column_sizes[i] < 0)
      grid.templateColumns.add(Grid::Px(-_dimension.column_sizes[i]));
    else 
    { 
      // autosize, dont bother with span
      int max_row_width = 0;
      for (int p = 0; p < _positions.size(); p++)
        if (_positions[p].row == _autofit_row && _positions[p].column == i)
        {
          auto autofit_child = dynamic_cast<autofit_component*>(getChildComponent(p));
          assert(autofit_child);
          auto fixed_width = autofit_child->fixed_width(col_width_even_distrib, row_height_even_distrib);
          assert(fixed_width > 0);
          max_row_width = std::max(max_row_width, fixed_width);
        }
      grid.templateColumns.add(Grid::Px(max_row_width));
    }

  for (int i = 0; i < _positions.size(); i++)
  {
    GridItem item(getChildComponent(i));
    item.row.start = _positions[i].row + 1;
    item.column.start = _positions[i].column + 1;
    item.row.end = _positions[i].row + 1 + _positions[i].row_span;
    item.column.end = _positions[i].column + 1 + _positions[i].column_span;
    grid.items.add(item);
  }

  grid.performLayout(getLocalBounds());
}

}