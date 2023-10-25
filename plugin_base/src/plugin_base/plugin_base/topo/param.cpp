#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>
#include <set>

namespace plugin_base {

void 
param_dsp::validate() const
{
  if (automate == param_automate::automate)
    assert(direction == param_direction::input);

  if (direction == param_direction::output)
  {
    assert(rate == param_rate::block);
    assert(automate == param_automate::none);
  }

  if (rate == param_rate::accurate)
  {
    assert(automate != param_automate::none);
    assert(direction == param_direction::input);
  }

  if (automate == param_automate::modulate)
  {
    assert(rate == param_rate::accurate);
    assert(direction == param_direction::input);
  }
}

void
param_topo_gui::validate(module_topo const& module, param_topo const& param) const
{
  bindings.validate(module, param.info.slot_count);
  position.validate(module.sections[section].gui.dimension);

  for (int e = 0; e < bindings.enabled.params.size(); e++)
    assert(param.info.index != bindings.enabled.params[e]);
  for (int v = 0; v < bindings.visible.params.size(); v++)
    assert(param.info.index != bindings.visible.params[v]);

  if (submenus.size())
  {
    assert(is_list());
    auto all_indices = vector_join(vector_map(submenus, [](auto const& s) { return s.indices; }));
    std::set<int> indices_set(all_indices.begin(), all_indices.end());
    assert(all_indices.size() == indices_set.size());
    assert(*all_indices.begin() == 0);
    assert(*(all_indices.end() - 1) == param.domain.max);
    for(int i = 0; i < submenus.size(); i++)
      submenus[i].validate();
  }
}

void
param_topo::validate(module_topo const& module, int index) const
{
  dsp.validate();
  info.validate();
  domain.validate();

  if (domain.type != domain_type::dependent)
  {
    assert(domain.max > domain.min);
    assert(dependent.selector == nullptr);
    assert(dependent.domains.size() == 0);
    assert(dependent.dependencies.size() == 0);
    assert(gui.edit_type != gui_edit_type::dependent);
  }
  else
  {
    int max = -1;
    assert(dependent.domains.size() > 1);
    assert(dependent.selector != nullptr);
    assert(dependent.dependencies.size() > 0);
    assert(gui.edit_type == gui_edit_type::dependent);
    assert(dependent.dependencies.size() <= max_param_dependencies_count);

    for (int d = 0; d < dependent.dependencies.size(); d++)
    {
      assert(dependent.dependencies[d] < index);
      auto const& other = module.params[dependent.dependencies[d]];
      (void)other;
      assert(other.domain.min == 0);
      assert(!other.domain.is_real());
      assert(info.slot_count == other.info.slot_count);
      assert(other.dsp.direction != param_direction::output);
    }

    for(int i = 0; i < dependent.domains.size(); i++)
    {
      max = std::max(max, (int)dependent.domains[i].max);
      dependent.domains[i].validate();
      assert(dependent.domains[i].min == 0);
      assert(dependent.domains[i].type == domain_type::item
          || dependent.domains[i].type == domain_type::name
          || dependent.domains[i].type == domain_type::step);
    }
    assert(max == domain.max);
  }

  assert(info.index == index);
  assert(domain.is_real() || dsp.rate == param_rate::block);
  assert(0 <= gui.section && gui.section < module.sections.size());
  assert((info.slot_count == 1) == (gui.layout == gui_layout::single));
  assert(gui.edit_type != gui_edit_type::toggle || domain.type == domain_type::toggle);
  assert(dsp.direction != param_direction::output || dependent.dependencies.size() == 0);
  assert(dsp.direction == param_direction::input || gui.bindings.enabled.selector == nullptr);
  assert(dsp.direction != param_direction::output || module.dsp.stage == module_stage::output);
}

}