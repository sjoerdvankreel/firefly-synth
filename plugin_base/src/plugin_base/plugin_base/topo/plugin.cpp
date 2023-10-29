#include <plugin_base/topo/plugin.hpp>
#include <set>

namespace plugin_base {

void 
module_section_gui::validate(plugin_topo const& plugin, int index_) const
{
  assert(this->index == index_);
  auto always_visible = [](int) {return true; };
  auto include = [this, &plugin](int m) { return plugin.modules[m].gui.section == this->index; };
  position.validate(plugin.gui.dimension); 
  dimension.validate(vector_map(plugin.modules, [](auto const& p) { return p.gui.position; }), include, always_visible);
}

void
plugin_topo::validate() const
{
  assert(modules.size());
  assert(extension.size());
  assert(version_major >= 0);
  assert(version_minor >= 0);
  assert(gui.default_width <= 3840);
  assert(gui.typeface_file_name.size());
  assert(polyphony >= 0 && polyphony < topo_max);
  assert(0 < gui.aspect_ratio_width && gui.aspect_ratio_width <= 100);
  assert(0 < gui.aspect_ratio_height && gui.aspect_ratio_height <= 100);
  assert(0 < gui.sections.size() && gui.sections.size() <= modules.size());
  assert(0 < gui.min_width && gui.min_width <= gui.default_width && gui.default_width <= gui.max_width);

  tag.validate();
  auto return_true = [](int) { return true; };
  gui.dimension.validate(vector_map(gui.sections, [](auto const& s) { return s.position; }), return_true, return_true);
  for(int s = 0; s < gui.sections.size(); s++)
    gui.sections[s].validate(*this, s);

  int stage = 0;
  std::set<std::string> module_ids;
  for (int m = 0; m < modules.size(); m++)
  {
    modules[m].validate(*this, m);
    assert((int)modules[m].dsp.stage >= stage);
    stage = (int)modules[m].dsp.stage;
    
    std::set<std::string> param_ids;
    std::set<std::string> section_ids;
    INF_ASSERT_EXEC(module_ids.insert(modules[m].info.tag.id).second);
    for (int s = 0; s < modules[m].sections.size(); s++)
      INF_ASSERT_EXEC(section_ids.insert(modules[m].sections[s].tag.id).second);
    for (int p = 0; p < modules[m].params.size(); p++)
      INF_ASSERT_EXEC(param_ids.insert(modules[m].params[p].info.tag.id).second);
  }
}

}