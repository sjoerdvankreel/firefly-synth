#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>

#include <set>
#include <utility>
#include <cassert>

namespace plugin_base {

void
topo_tag::validate() const
{
  assert(id.size());
  assert(name.size());
}

void
topo_info::validate() const
{
  assert(0 <= index && index < topo_max);
  assert(0 < slot_count && slot_count < topo_max);
  tag.validate();
}

void
gui_submenu::validate() const
{
  assert(name.size());
  assert(indices.size());
  for(int i = 0; i < indices.size(); i++)
    assert(indices[i] >= 0);
  std::set<int> indices_set(indices.begin(), indices.end());
  assert(indices_set.size() == indices.size());
}

void
gui_bindings::validate(module_topo const& module, int slot_count) const
{
  enabled.validate(module, slot_count);
  visible.validate(module, slot_count);
}

void
gui_position::validate(gui_dimension const& parent_dimension) const
{
  assert(0 <= row && row < topo_max);
  assert(0 <= column && column < topo_max);
  assert(0 < row_span && row_span < topo_max);
  assert(0 < column_span && column_span < topo_max);
  assert(row + row_span <= parent_dimension.row_sizes.size());
  assert(column + column_span <= parent_dimension.column_sizes.size());
}

void
gui_binding::validate(module_topo const& module, int slot_count) const
{
  assert((params.size() == 0) == (selector == nullptr));
  for (int i = 0; i < params.size(); i++)
  {
    auto const& bound = module.params[params[i]];
    (void)bound;
    assert(!bound.domain.is_real());
    assert(bound.info.slot_count == 1 || bound.info.slot_count == slot_count);
  }
}

void
gui_dimension::validate(
  std::vector<gui_position> const& children,
  std::function<bool(int)> include,
  std::function<bool(int)> always_visible) const
{
  std::set<std::pair<int, int>> taken;
  assert(0 < row_sizes.size() && row_sizes.size() < topo_max);
  assert(0 < column_sizes.size() && column_sizes.size() < topo_max);

  for (int k = 0; k < children.size(); k++)
  {
    if (!include(k)) continue;
    auto const& pos = children[k];
    for (int r = pos.row; r < pos.row + pos.row_span; r++)
      for (int c = pos.column; c < pos.column + pos.column_span; c++)
        INF_ASSERT_EXEC(taken.insert(std::make_pair(r, c)).second || !always_visible(k));
  }
  for (int r = 0; r < row_sizes.size(); r++)
    for (int c = 0; c < column_sizes.size(); c++)
      assert(taken.find(std::make_pair(r, c)) != taken.end());
}

}