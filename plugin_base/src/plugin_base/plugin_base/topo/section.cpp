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
  gui.dimension.validate(vector_map(module.params, [](auto const& p){ return p.gui.position; }), include, always_visible);
}

}