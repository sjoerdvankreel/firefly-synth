#include <plugin_base/topo/plugin.hpp>
#include <set>

namespace plugin_base {

void
plugin_topo::validate() const
{
  assert(modules.size());
  assert(extension.size());
  assert(version_major >= 0);
  assert(version_minor >= 0);
  assert(gui.default_width <= 3840);
  assert(polyphony >= 0 && polyphony < topo_max);
  assert(0 < gui.aspect_ratio_width && gui.aspect_ratio_width <= 100);
  assert(0 < gui.aspect_ratio_height && gui.aspect_ratio_height <= 100);
  assert(0 < gui.min_width && gui.min_width <= gui.default_width && gui.default_width <= gui.max_width);

  tag.validate();
  auto return_true = [](int) { return true; };
  gui.dimension.validate(map_vector(modules, [](auto const& m) { return m.gui.position; }), return_true, return_true);

  int stage = 0;
  std::set<std::string> all_ids;
  for (int m = 0; m < modules.size(); m++)
  {
    modules[m].validate(*this, m);
    assert((int)modules[m].dsp.stage >= stage);
    stage = (int)modules[m].dsp.stage;
    INF_ASSERT_EXEC(all_ids.insert(modules[m].info.tag.id).second);
    for (int s = 0; s < modules[m].sections.size(); s++)
      INF_ASSERT_EXEC(all_ids.insert(modules[m].sections[s].tag.id).second);
    for (int p = 0; p < modules[m].params.size(); p++)
      INF_ASSERT_EXEC(all_ids.insert(modules[m].params[p].info.tag.id).second);
  }
}

}