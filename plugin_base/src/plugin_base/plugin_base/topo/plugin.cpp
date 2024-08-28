#include <plugin_base/topo/plugin.hpp>
#include <set>

namespace plugin_base {

void 
custom_section_gui::validate(plugin_topo const& plugin, int index_) const
{
  assert(index == index_);
  assert(full_name.size());
  assert(gui_factory != nullptr);
  position.validate(plugin.gui.dimension_factory(plugin_topo_gui_theme_settings()));
}

void 
module_section_gui::validate(plugin_topo const& plugin, int index_) const
{
  assert(id.size());
  assert(this->index == index_);
  if(!visible) return;
  auto always_visible = [](int) {return true; };
  auto include = [this, &plugin](int m) { return plugin.modules[m].gui.section == this->index; };
  position.validate(plugin.gui.dimension_factory(plugin_topo_gui_theme_settings())); 
  if (tabbed) 
  {
    assert(tab_order.size());
    assert(dimension.column_sizes.size() == 1 && dimension.row_sizes.size() == 1);
  } else
    dimension.validate(gui_label_edit_cell_split::no_split, vector_map(plugin.modules, [](auto const& p) { return p.gui.position; }), {}, include, always_visible);
}

void
plugin_topo::validate() const
{
  assert(vendor.size());
  assert(modules.size());
  assert(extension.size());
  assert(version.major >= 0);
  assert(version.minor >= 0);
  assert(version.patch >= 0);
  assert(audio_polyphony >= 0 && audio_polyphony < topo_max);
  assert(graph_polyphony >= 0 && graph_polyphony < topo_max);

  tag.validate();
  assert(gui.default_theme.size());
  assert(gui.dimension_factory != nullptr);

  // need to validate module and custom sections together
  auto return_true = [](int) { return true; };
  std::vector<std::pair<gui_position, bool>> all_sections;
  for(int i = 0; i < gui.custom_sections.size(); i++)
    all_sections.push_back(std::make_pair(gui.custom_sections[i].position, true));
  for (int i = 0; i < gui.module_sections.size(); i++)
    all_sections.push_back(std::make_pair(gui.module_sections[i].position, gui.module_sections[i].visible));
  gui.dimension_factory(plugin_topo_gui_theme_settings()).validate(gui_label_edit_cell_split::no_split, 
    vector_map(all_sections, [](auto const& s) { return s.first; }), {},
    [&all_sections](int i) { return all_sections[i].second; }, return_true);
  
  for(int s = 0; s < gui.custom_sections.size(); s++)
    gui.custom_sections[s].validate(*this, s);
  for(int s = 0; s < gui.module_sections.size(); s++)
    gui.module_sections[s].validate(*this, s);

  assert((engine.voice_mode.module_index == -1) == (engine.voice_mode.param_index == -1));
  assert((engine.tuning_mode.module_index == -1) == (engine.tuning_mode.param_index == -1));
  assert((engine.bpm_smoothing.module_index == -1) == (engine.bpm_smoothing.param_index == -1));
  assert((engine.midi_smoothing.module_index == -1) == (engine.midi_smoothing.param_index == -1));
  assert((engine.automation_smoothing.module_index == -1) == (engine.automation_smoothing.param_index == -1));

  int stage = 0;
  std::set<std::string> module_ids;
  for (int m = 0; m < modules.size(); m++)
  {
    modules[m].validate(*this, m);
    assert((int)modules[m].dsp.stage >= stage);
    stage = (int)modules[m].dsp.stage;
    
    std::set<std::string> param_ids;
    std::set<std::string> output_ids;
    std::set<std::string> section_ids;
    PB_ASSERT_EXEC(module_ids.insert(modules[m].info.tag.id).second);
    for (int s = 0; s < modules[m].sections.size(); s++)
      PB_ASSERT_EXEC(section_ids.insert(modules[m].sections[s].tag.id).second);
    for (int p = 0; p < modules[m].params.size(); p++)
      PB_ASSERT_EXEC(param_ids.insert(modules[m].params[p].info.tag.id).second);
    for (int o = 0; o < modules[m].dsp.outputs.size(); o++)
      PB_ASSERT_EXEC(output_ids.insert(modules[m].dsp.outputs[o].info.tag.id).second);
    if (gui.module_sections[modules[m].gui.section].tabbed) assert(modules[m].info.slot_count == 1);
    assert(!gui.module_sections[modules[m].gui.section].tabbed || modules[m].gui.tabbed_name.size());
  }
}

std::vector<std::string>
plugin_topo::themes() const
{
  // expect theme folders directly in the "themes" folder
  std::vector<std::string> result;
  auto themes_folder = get_resource_location(config) / resource_folder_themes;
  if (!std::filesystem::exists(themes_folder)) return {};
  for (auto const& entry : std::filesystem::directory_iterator{ themes_folder })
    if (entry.is_directory())
      result.push_back(entry.path().filename().string());
  std::sort(result.begin(), result.end(), [](auto const& l, auto const& r) { return l < r; });
  return result;
}

std::vector<list_item>
plugin_topo::preset_list() const
{
  auto items = presets();
  std::vector<list_item> result;
  for (int i = 0; i < items.size(); i++)
    result.emplace_back(items[i].name, items[i].name);
  return result;
}

std::shared_ptr<gui_submenu>
plugin_topo::preset_submenu() const
{
  auto items = presets();
  std::map<std::string, std::vector<int>> presets_by_group;
  for (int i = 0; i < items.size(); i++)
    presets_by_group[items[i].group].push_back(i);
  auto result = std::make_shared<gui_submenu>();
  for (auto const& pbg : presets_by_group)
  {
    auto sub = std::make_shared<gui_submenu>();
    sub->name = pbg.first;
    sub->indices = pbg.second;
    result->children.push_back(sub);
  }
  return result;
}

std::vector<preset_item>
plugin_topo::presets() const
{
  // expect preset files nested 1 level deep, subfolders act as grouping
  std::vector<preset_item> result;
  auto preset_folder = get_resource_location(config) / resource_folder_presets;
  if (!std::filesystem::exists(preset_folder)) return {};
  for (auto const& group_entry : std::filesystem::directory_iterator{ preset_folder })
    if (group_entry.is_directory())
      for (auto const& entry : std::filesystem::directory_iterator{ group_entry.path() })
        if (entry.is_regular_file() && entry.path().extension().string() == std::string(".") + extension)
        {
          preset_item item;
          item.path = entry.path().string();
          item.name = entry.path().stem().string();
          item.group = group_entry.path().filename().string();
          result.push_back(item);
        }
  std::sort(result.begin(), result.end(), [](auto const& l, auto const& r) {
    if (l.group < r.group) return true;
    if (r.group < l.group) return false;
    return l.name < r.name;
    });
  return result;
}

}