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

int 
grid_component::fixed_width() const
{
  // ignore span
  int result = 0;
  for(int c = 0; c < _dimension.column_sizes.size(); c++)
    for (int i = 0; i < _positions.size(); i++)
      if(_positions[i].column == c)
        if(_positions[i].row == 0)
        {
          auto& child = dynamic_cast<autofit_component&>(*getChildComponent(i));
          assert(child.fixed_width() > 0);
          result += child.fixed_width();
        }
  return result + (_dimension.column_sizes.size() - 1) * _gap_size;
}

int 
grid_component::fixed_height() const
{
  // ignore span
  int result = 0;
  for (int r = 0; r < _dimension.row_sizes.size(); r++)
    for (int i = 0; i < _positions.size(); i++)
      if (_positions[i].row == r)
        if (_positions[i].column == 0)
        {
          auto& child = dynamic_cast<autofit_component&>(*getChildComponent(i));
          assert(child.fixed_height() > 0);
          result += child.fixed_height();
        }
  return result + (_dimension.row_sizes.size() - 1) * _gap_size;
}

void 
grid_component::resized()
{
  Grid grid;
  grid.rowGap = Grid::Px(_gap_size);
  grid.columnGap = Grid::Px(_gap_size);

  for(int i = 0; i < _dimension.row_sizes.size(); i++)
    if(_dimension.row_sizes[i] > 0)
      grid.templateRows.add(Grid::Fr(_dimension.row_sizes[i]));
    else if (_dimension.row_sizes[i] < 0)
      grid.templateRows.add(Grid::Px(-_dimension.row_sizes[i]));
    else 
    {
      // autosize, dont bother with span
      int max_col_height = 0;
      for (int c = 0; c < _dimension.column_sizes.size(); c++)
        for(int p = 0; p < _positions.size(); p++)
          if(_positions[p].column == c && _positions[p].row == i)
          {
            auto autofit_child = dynamic_cast<autofit_component*>(getChildComponent(p));
            assert(autofit_child && autofit_child->fixed_height() > 0);
            max_col_height = std::max(max_col_height, autofit_child->fixed_height());
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
      for (int r = 0; r < _dimension.row_sizes.size(); r++)
        for (int p = 0; p < _positions.size(); p++)
          if (_positions[p].row == r && _positions[p].column == i)
          {
            auto autofit_child = dynamic_cast<autofit_component*>(getChildComponent(p));
            assert(autofit_child && autofit_child->fixed_width() > 0);
            max_row_width = std::max(max_row_width, autofit_child->fixed_width());
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