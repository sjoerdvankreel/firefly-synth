#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <cassert>

namespace plugin_base {

void
component_tag::validate() const
{
  assert(id.size());
  assert(name.size());
}

void
component_info::validate() const
{
  assert(0 <= index && index < topo_max);
  assert(0 < slot_count && slot_count < topo_max);
  tag.validate();
}

void
gui_position::validate() const
{
  assert(0 <= row && row < topo_max);
  assert(0 <= column && column < topo_max);
  assert(0 < row_span && row_span < topo_max);
  assert(0 < column_span && column_span < topo_max);
}

void
gui_dimension::validate() const
{
  assert(0 < row_sizes.size() && row_sizes.size() < topo_max);
  assert(0 < column_sizes.size() && column_sizes.size() < topo_max);
}

void
gui_bindings::validate(module_topo const& module, int slot_count) const
{
  enabled.validate(module, slot_count);
  visible.validate(module, slot_count);
}

void
gui_binding::validate(module_topo const& module, int slot_count) const
{
  assert((params.size() == 0) == (selector == nullptr));
  assert((context.size() == 0) || (context.size() == params.size()));
  for (int i = 0; i < params.size(); i++)
  {
    auto const& bound = module.params[params[i]];
    assert(!bound.domain.is_real());
    assert(bound.info.slot_count == 1 || bound.info.slot_count == slot_count);
  }
}

}