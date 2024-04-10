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
      if (module.params[p].gui.layout == param_layout::single_grid)
      {
        assert(!is_one_grid_param);
        is_one_grid_param = true;
      }
    }

  // validation is handled elsewhere
  if(is_one_grid_param) return;

  gui.dimension.validate(gui.cell_split, 
    vector_map(module.params, [](auto const& p){ return p.gui.position; }), 
    vector_map(module.params, [](auto const& p){ return p.gui.label.contents; }),
    include, always_visible);
}

}