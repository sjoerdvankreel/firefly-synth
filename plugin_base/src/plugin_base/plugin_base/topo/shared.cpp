#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/plugin.hpp>

#include <set>
#include <cmath>
#include <utility>
#include <cassert>

namespace plugin_base {

void
topo_tag::validate() const
{
  assert(id.size());
  assert(name.size());
  assert(short_name.size() <= name.size());
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
  if (is_subheader)
  {
    assert(name.size());
    assert(!indices.size());
    assert(!children.size());
    return;
  }
  assert(indices.size());
  assert(name.size() || children.size());
  for(int i = 0; i < indices.size(); i++)
    assert(indices[i] >= 0);
  for(int i = 0; i < children.size(); i++)
    children[i]->validate();
}

void
gui_submenu::add_subheader(std::string const& name_)
{
  auto result = std::make_shared<gui_submenu>();
  result->name = name_;
  result->is_subheader = true;
  children.push_back(result);
}

std::shared_ptr<gui_submenu> 
gui_submenu::add_submenu(std::string const& name_)
{
  auto result = std::make_shared<gui_submenu>();
  result->name = name_;
  children.push_back(result);
  return result;
}

void 
gui_submenu::add_submenu(std::string const& name_, std::vector<int> const& indices_)
{
  auto submenu = add_submenu(name_);
  for(int index: indices_)
    submenu->indices.push_back(index);
}

void
gui_binding::bind_slot(gui_slot_binding_selector selector_)
{
  assert(selector_ != nullptr);
  assert(slot_selector == nullptr);
  slot_selector = selector_;
}

void 
gui_binding::bind_params(std::vector<int> const& params_, gui_param_binding_selector selector_)
{
  assert(params_.size());
  assert(selector_ != nullptr);
  assert(param_selector == nullptr);
  params = params_;
  param_selector = selector_;
}

void 
gui_global_binding::bind_param(int module_, int param_, gui_global_param_binding_selector selector_)
{
  assert(param_ >= 0);
  assert(module_ >= 0);
  assert(selector_ != nullptr);
  assert(selector == nullptr);
  param = param_;
  module = module_;
  selector = selector_;
}

void
gui_bindings::validate(plugin_topo const& plugin, module_topo const& module, int slot_count) const
{
  global_enabled.validate(plugin);
  global_enabled.validate(plugin);
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
  assert((params.size() == 0) == (param_selector == nullptr));
  for (int i = 0; i < params.size(); i++)
  {
    auto const& bound = module.params[params[i]];
    (void)bound;
    assert(!bound.domain.is_real());
    assert(bound.info.slot_count == 1 || bound.info.slot_count == slot_count);
  }
}

void
gui_global_binding::validate(plugin_topo const& plugin) const
{ 
  assert((param == -1) == (selector == nullptr));
  assert((module == -1) == (selector == nullptr));
  if(module == -1) return;
  auto const& bound = plugin.modules[module].params[param];
  (void)bound;
  assert(plugin.modules[module].info.slot_count == 1);
  assert(!bound.domain.is_real());
  assert(bound.info.slot_count == 1);
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
        PB_ASSERT_EXEC(taken.insert(std::make_pair(r, c)).second || !always_visible(k));
  }
  for (int r = 0; r < row_sizes.size(); r++)
    for (int c = 0; c < column_sizes.size(); c++)
      assert(taken.find(std::make_pair(r, c)) != taken.end());
}

}