#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/section.hpp>
#include <plugin_base/shared/utility.hpp>

namespace plugin_base {

void 
param_section::validate(plugin_topo const& plugin, module_topo const& module, int index_) const
{
  tag.validate();
  gui.bindings.validate(plugin, module, 1);
  gui.position.validate(module.gui.dimension);
   
  assert(this->index == index_);
  auto always_visible = [&module](int p) { return !module.params[p].gui.bindings.visible.is_bound(); };
  auto include = [this, &module](int p) { return module.params[p].gui.visible && module.params[p].gui.section == this->index; };

  bool is_one_grid_param = false;
  for(int p = 0; p < module.params.size(); p++)
    if (include(p))
    {
      if (module.params[p].gui.layout == param_layout::parent_grid)
      {
        assert(!is_one_grid_param);
        is_one_grid_param = true;
      }
    }

  // blend/merge should be 2-way to simplify the ui code
  if (gui.merge_with_section != -1)
  {
    auto const& that_gui = module.sections[gui.merge_with_section].gui;
    assert(that_gui.merge_with_section == index);
    if (gui.position.row == that_gui.position.row)
      assert(gui.position.column + gui.position.column_span == that_gui.position.column ||
        that_gui.position.column + that_gui.position.column_span == gui.position.column);
    else if (gui.position.column == that_gui.position.column)
      assert(gui.position.row + gui.position.row_span == that_gui.position.row ||
        that_gui.position.row + that_gui.position.row_span == gui.position.row);
    else
      assert(false);
  }

  if (gui.custom_gui_factory != nullptr)
  {
    assert(!is_one_grid_param);
    assert(gui.wrap_in_container);
    assert(gui.autofit_row == 0);
    assert(gui.autofit_column == 0);
    assert(gui.scroll_mode == gui_scroll_mode::none);
    assert(gui.cell_split == gui_label_edit_cell_split::no_split);
    assert(gui.dimension.row_sizes.size() == 1 && gui.dimension.row_sizes[0] == 1);
    assert(gui.dimension.column_sizes.size() == 1 && gui.dimension.column_sizes[0] == 1);
    return;
  }

  // validation is handled elsewhere
  if(is_one_grid_param) return;

  gui.dimension.validate(gui.cell_split, 
    vector_map(module.params, [](auto const& p){ return p.gui.position; }), 
    vector_map(module.params, [](auto const& p){ return p.gui.label.contents; }),
    include, always_visible);
}

}