#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/section.hpp>

namespace plugin_base {

void 
section_topo::validate(module_topo const& module) const
{
  tag.validate();
  gui.bindings.validate(module, 1);
  gui.position.validate(module.gui.dimension);

  assert(0 <= index && index < module.sections.size());
  auto include = [this, &module](int p) { return module.params[p].section == index; };
  auto always_visible = [&module](int p) { return module.params[p].gui.bindings.visible.selector == nullptr; };
  gui.dimension.validate(module.params, include, always_visible);
}

}