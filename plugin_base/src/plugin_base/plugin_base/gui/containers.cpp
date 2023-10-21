#include <plugin_base/gui/containers.hpp>

using namespace juce;

namespace plugin_base {

static inline int constexpr groupbox_padding = 6;
static inline int constexpr groupbox_padding_top = 16;

void
group_component::resized()
{
  Rectangle<int> bounds(getLocalBounds());
  Rectangle<int> child_bounds(
    bounds.getX() + groupbox_padding, 
    bounds.getY() + groupbox_padding_top, 
    bounds.getWidth() - 2 * groupbox_padding, 
    bounds.getHeight() - groupbox_padding - groupbox_padding_top);
  assert(getNumChildComponents() < 2);
  if (getNumChildComponents() == 1)
    getChildComponent(0)->setBounds(child_bounds);
}

void
grid_component::add(Component& child, gui_position const& position)
{
  add_and_make_visible(*this, child);
  _positions.push_back(position);
}

void 
grid_component::resized()
{
  Grid grid;

  for(int i = 0; i < _dimension.row_sizes.size(); i++)
    if(_dimension.row_sizes[i] > 0)
      grid.templateRows.add(Grid::Fr(_dimension.row_sizes[i]));
    else if (_dimension.row_sizes[i] < 0)
      grid.templateRows.add(Grid::Px(-_dimension.row_sizes[i]));
    else
      assert(false);

  for(int i = 0; i < _dimension.column_sizes.size(); i++)
    if(_dimension.column_sizes[i] > 0)
      grid.templateColumns.add(Grid::Fr(_dimension.column_sizes[i]));
    else if(_dimension.column_sizes[i] < 0)
      grid.templateColumns.add(Grid::Px(-_dimension.column_sizes[i]));
    else
      assert(false);

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