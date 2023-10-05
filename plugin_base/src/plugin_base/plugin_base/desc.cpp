#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>

#include <set>

namespace plugin_base {

static std::string
param_id(param_topo const& param, int slot)
{
  std::string result = param.info.tag.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
module_id(module_topo const& module, int slot)
{
  std::string result = module.info.tag.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
param_name(param_topo const& param, int slot)
{
  std::string result = param.info.tag.name;
  if (param.info.slot_count > 1) result += " " + std::to_string(slot + 1);
  return result;
}

static std::string
module_name(module_topo const& module, int slot)
{
  std::string result = module.info.tag.name;
  if (module.info.slot_count > 1) result += " " + std::to_string(slot + 1);
  return result;
}

// nonnegative required for vst3 param tags
static int
stable_hash(std::string const& text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text.c_str());
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

static void
validate_dims(plugin_topo const& plugin, plugin_dims const& dims)
{
  assert(dims.voice_module_slot.size() == plugin.polyphony);
  for(int v = 0; v < plugin.polyphony; v++)
  {
    assert(dims.voice_module_slot[v].size() == plugin.modules.size());
    for(int m = 0; m < plugin.modules.size(); m++)
      assert(dims.voice_module_slot[v][m] == plugin.modules[m].info.slot_count);
  }

  assert(dims.module_slot.size() == plugin.modules.size());
  assert(dims.module_slot_param_slot.size() == plugin.modules.size());
  for (int m = 0; m < plugin.modules.size(); m++)
  {    
    auto const& module = plugin.modules[m];
    assert(dims.module_slot[m] == module.info.slot_count);
    assert(dims.module_slot_param_slot[m].size() == module.info.slot_count);
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(dims.module_slot_param_slot[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
        assert(dims.module_slot_param_slot[m][mi][p] == module.params[p].info.slot_count);
    }
  }
}

static void
validate_frame_dims(
  plugin_topo const& plugin, plugin_frame_dims const& dims, int frame_count)
{
  assert(dims.audio.size() == 2);
  assert(dims.audio[0] == frame_count);
  assert(dims.audio[1] == frame_count);

  assert(dims.voices_audio.size() == plugin.polyphony);
  assert(dims.module_voice_cv.size() == plugin.polyphony);
  assert(dims.module_voice_audio.size() == plugin.polyphony);
  for (int v = 0; v < plugin.polyphony; v++)
  {
    assert(dims.voices_audio[v].size() == 2);
    assert(dims.voices_audio[v][0] == frame_count);
    assert(dims.voices_audio[v][1] == frame_count);
    assert(dims.module_voice_cv[v].size() == plugin.modules.size());
    assert(dims.module_voice_audio[v].size() == plugin.modules.size());
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      bool is_cv = module.dsp.output == module_output::cv;
      bool is_voice = module.dsp.stage == module_stage::voice;
      bool is_audio = module.dsp.output == module_output::audio;
      int cv_frames = is_cv && is_voice ? frame_count : 0;
      int audio_frames = is_audio && is_voice ? frame_count : 0;
      (void)cv_frames;
      (void)audio_frames;
      assert(dims.module_voice_cv[v][m].size() == module.info.slot_count);
      assert(dims.module_voice_audio[v][m].size() == module.info.slot_count);
      for (int mi = 0; mi < module.info.slot_count; mi++)
      {
        assert(dims.module_voice_cv[v][m][mi].size() == module.dsp.output_count);
        assert(dims.module_voice_audio[v][m][mi].size() == module.dsp.output_count);

        for(int oi = 0; oi < module.dsp.output_count; oi++)
        {
          assert(dims.module_voice_cv[v][m][mi][oi] == cv_frames);
          assert(dims.module_voice_audio[v][m][mi][oi].size() == 2);
          for(int c = 0; c < 2; c++)
            assert(dims.module_voice_audio[v][m][mi][oi][c] == audio_frames);
        }
      }
    }
  }

  assert(dims.module_global_cv.size() == plugin.modules.size());
  assert(dims.module_global_audio.size() == plugin.modules.size());
  assert(dims.accurate_automation.size() == plugin.modules.size());
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    bool is_cv = module.dsp.output == module_output::cv;
    bool is_global = module.dsp.stage != module_stage::voice;
    bool is_audio = module.dsp.output == module_output::audio;
    int cv_frames = is_cv && is_global ? frame_count : 0;
    int audio_frames = is_audio && is_global ? frame_count : 0;
    (void)cv_frames;
    (void)audio_frames;
    assert(dims.module_global_cv[m].size() == module.info.slot_count);
    assert(dims.module_global_audio[m].size() == module.info.slot_count);
    assert(dims.accurate_automation[m].size() == module.info.slot_count);

    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(dims.module_global_cv[m][mi].size() == module.dsp.output_count);
      assert(dims.module_global_audio[m][mi].size() == module.dsp.output_count);

      for(int oi = 0; oi < module.dsp.output_count; oi++)
      {
        assert(dims.module_global_cv[m][mi][oi] == cv_frames);
        assert(dims.module_global_audio[m][mi][oi].size() == 2);
        for(int c = 0; c < 2; c++)
          assert(dims.module_global_audio[m][mi][oi][c] == audio_frames);
      }

      assert(dims.accurate_automation[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        int accurate_frames = param.dsp.rate == param_rate::accurate? frame_count: 0;
        (void)accurate_frames;
        assert(dims.accurate_automation[m][mi][p].size() == param.info.slot_count);
        for(int pi = 0; pi < param.info.slot_count; pi++)
          assert(dims.accurate_automation[m][mi][p][pi] == accurate_frames);
      }
    }
  }
}

template <class Parent, class Child, class VisibilitySelector, class Include>
static void validate_gui_layout(
  Parent const& parent, std::vector<Child> const& children,
  VisibilitySelector selector, Include include)
{
  std::set<std::pair<int, int>> gui_taken;
  for (int k = 0; k < children.size(); k++)
    if (include(children[k]))
    {
      auto const& pos = children[k].gui.position;
      for (int r = pos.row; r < pos.row + pos.row_span; r++)
        for (int c = pos.column; c < pos.column + pos.column_span; c++)
          INF_ASSERT_EXEC(gui_taken.insert(std::make_pair(r, c)).second
            || selector(children[k]) != nullptr);
    }
  for (int r = 0; r < parent.gui.dimension.row_sizes.size(); r++)
    for (int c = 0; c < parent.gui.dimension.column_sizes.size(); c++)
      assert(gui_taken.find(std::make_pair(r, c)) != gui_taken.end());
}

static void
validate_section_topo(module_topo const& module, section_topo const& section)
{
  section.tag.validate();
  assert(0 <= section.index && section.index < module.sections.size());
  section.gui.dimension.validate();
  section.gui.position.validate(module.gui.dimension);
  section.gui.bindings.validate(module, 1);
  validate_gui_layout(section, module.params,
    [](param_topo const& p) { return p.gui.bindings.visible.selector; },
    [&section](param_topo const& p) { return p.section == section.index; });
}

static void
validate_module_topo(plugin_topo const& plugin, module_topo const& module)
{
  module.info.validate();
  assert(module.params.size());
  assert(module.engine_factory);
  assert(module.dsp.output == module_output::none || module.dsp.output_count > 0);
  assert(module.dsp.output != module_output::none || module.dsp.output_count == 0);
  assert(0 < module.sections.size() && module.sections.size() <= module.params.size());
  assert((module.info.slot_count == 1) == (module.gui.layout == gui_layout::single));
  module.gui.dimension.validate();
  module.gui.position.validate(plugin.gui.dimension);
  validate_gui_layout(module, module.sections, [](auto const& section) { return section.gui.bindings.visible.selector; }, [](auto const&) { return true; });

  for(int p = 0; p < module.params.size(); p++)
  {
    auto const& param = module.params[p];
    for (int e = 0; e < param.gui.bindings.enabled.params.size(); e++)
      assert(param.info.index != param.gui.bindings.enabled.params[e]);
    for(int v = 0; v < param.gui.bindings.visible.params.size(); v++)
      assert(param.info.index != param.gui.bindings.visible.params[v]);
  }
}

static void
validate_param_topo(module_topo const& module, param_topo const& param)
{
  param.info.validate();
  assert(param.section >= 0);
  assert(param.section < module.sections.size());
  assert(0 <= param.section && param.section < module.sections.size());

  assert(param.dsp.format == param_format::plain || param.domain.is_real());
  assert(param.dsp.direction != param_direction::output || param.dsp.rate == param_rate::block);

  assert(param.domain.is_real() || param.dsp.rate == param_rate::block);
  assert(param.gui.edit_type != gui_edit_type::toggle || param.domain.type == domain_type::toggle);

  param.domain.validate();
  param.gui.bindings.validate(module, param.info.slot_count);
  assert(param.dsp.direction == param_direction::input || param.gui.bindings.enabled.selector == nullptr);
  assert((param.info.slot_count == 1) == (param.gui.layout == gui_layout::single));
  param.gui.position.validate(module.sections[param.section].gui.dimension);
}

static void
validate_plugin_topo(plugin_topo const& topo)
{
  std::set<std::string> param_ids;
  std::set<std::string> module_ids;
  validate_gui_layout(topo, topo.modules, [](auto const&) { return nullptr; }, [](auto const&) { return true; });

  topo.tag.validate();
  assert(topo.modules.size());
  assert(topo.version_major >= 0);
  assert(topo.version_minor >= 0);
  assert(topo.extension.size());
  assert(topo.gui.default_width <= 3840);
  assert(topo.polyphony >= 0 && topo.polyphony <= 1024);
  topo.gui.dimension.validate();

  assert(0 < topo.gui.aspect_ratio_width && topo.gui.aspect_ratio_width <= 100);
  assert(0 < topo.gui.aspect_ratio_height && topo.gui.aspect_ratio_height <= 100);
  assert(0 < topo.gui.dimension.row_sizes.size() && topo.gui.dimension.row_sizes.size() <= 1024);
  assert(0 < topo.gui.dimension.column_sizes.size() && topo.gui.dimension.column_sizes.size() <= 1024);
  assert(0 < topo.gui.min_width && topo.gui.min_width <= topo.gui.default_width && topo.gui.default_width <= topo.gui.max_width);

  int stage = 0;
  (void)stage;
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    assert((int)module.dsp.stage >= stage);
    stage = (int)module.dsp.stage;

    validate_module_topo(topo, module);
    INF_ASSERT_EXEC(module_ids.insert(module.info.tag.id).second);
    for (int s = 0; s < module.sections.size(); s++)
      validate_section_topo(module, module.sections[s]);
    for (int p = 0; p < module.params.size(); p++)
    {
      validate_param_topo(module, module.params[p]);
      INF_ASSERT_EXEC(param_ids.insert(module.params[p].info.tag.id).second);
    }
  }
}

static void
validate_module_desc(plugin_desc const& plugin_desc, module_desc const& desc)
{
  assert(desc.module);
  // TODO move to tag
  assert(desc.id.size());
  assert(desc.name.size());
  assert(desc.params.size());
  assert(desc.id_hash >= 0);
  assert(0 <= desc.slot && desc.slot < desc.module->info.slot_count);
  assert(0 <= desc.global && desc.global < plugin_desc.modules.size());
  assert(0 <= desc.topo && desc.topo < plugin_desc.plugin->modules.size());
}

static void
validate_param_desc(module_desc const& module_desc, param_desc const& desc)
{
  assert(desc.param);
  assert(desc.id_hash >= 0);
  assert(desc.id.size() > 0);
  assert(0 <= desc.slot && desc.slot < desc.param->info.slot_count);
  assert(0 <= desc.local && desc.local < module_desc.params.size());
  assert(0 <= desc.topo && desc.topo < module_desc.module->params.size());
  assert(0 < desc.name.size() && desc.name.size() < desc.full_name.size());
}

static void
validate_plugin_desc(plugin_desc const& desc)
{
  int param_global = 0;
  std::set<int> all_hashes;
  std::set<std::string> all_ids;
  (void)param_global;

  assert(0 <= desc.module_voice_start);
  assert(desc.module_voice_start <= desc.module_output_start);
  assert(desc.module_output_start < desc.plugin->modules.size());

  assert(desc.param_count > 0);
  assert(desc.module_count > 0);
  assert(desc.params.size() == desc.param_count);
  assert(desc.modules.size() == desc.module_count);
  assert(desc.mappings.size() == desc.param_count);
  assert(desc.param_tag_to_index.size() == desc.param_count);
  assert(desc.param_index_to_tag.size() == desc.param_count);
  assert(desc.param_id_to_index.size() == desc.plugin->modules.size());
  assert(desc.module_id_to_index.size() == desc.plugin->modules.size());
  assert(desc.param_topo_to_index.size() == desc.plugin->modules.size());

  param_global = 0;
  for(int m = 0; m < desc.plugin->modules.size(); m++)
  {
    auto const& module = desc.plugin->modules[m];
    (void)module;
    assert(desc.plugin->modules[m].info.index == m);
    assert(desc.param_topo_to_index[m].size() == module.info.slot_count);
    assert(desc.param_id_to_index.at(module.info.tag.id).size() == module.params.size());
    for(int s = 0; s < module.sections.size(); s++)
      assert(module.sections[s].index == s);
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(desc.param_topo_to_index[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        assert(param.info.index == p);
        assert(desc.param_topo_to_index[m][mi][p].size() == param.info.slot_count);
        for(int pi = 0; pi < param.info.slot_count; pi++)
          assert(desc.param_topo_to_index[m][mi][p][pi] == param_global++);
      }
    }
  }

  param_global = 0;
  for (int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    assert(module.global == m);
    validate_module_desc(desc, module);
    INF_ASSERT_EXEC(all_ids.insert(module.id).second);
    INF_ASSERT_EXEC(all_hashes.insert(module.id_hash).second);
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      validate_param_desc(module, param);
      assert(param.global == param_global++);
      INF_ASSERT_EXEC(all_ids.insert(param.id).second);
      INF_ASSERT_EXEC(all_hashes.insert(param.id_hash).second);
    }
  }
}

param_desc::
param_desc(
  module_topo const& module_, int module_slot,
  param_topo const& param_, int topo, int slot, int local, int global)
{
  param = &param_;
  this->topo = topo;
  this->slot = slot;
  this->local = local;
  this->global = global;
  name = param_name(param_, slot);
  full_name = module_name(module_, module_slot) + " " + name;
  id = module_id(module_, module_slot) + "-" + param_id(param_, slot);
  id_hash = stable_hash(id.c_str());
}

module_desc::
module_desc(
  module_topo const& module_, int topo, int slot, int global, int param_global_start)
{
  module = &module_;
  this->topo = topo;
  this->slot = slot;
  this->global = global;
  id = module_id(module_, slot);
  name = module_name(module_, slot);
  id_hash = stable_hash(id);

  int param_local = 0;
  for(int p = 0; p < module_.params.size(); p++)
    for(int i = 0; i < module_.params[p].info.slot_count; i++)
      params.emplace_back(param_desc(module_, slot, module_.params[p], p, i, param_local++, param_global_start++));
}

plugin_desc::
plugin_desc(std::unique_ptr<plugin_topo>&& plugin_):
plugin(std::move(plugin_))
{
  int param_global = 0;
  int module_global = 0;

  for(int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    param_topo_to_index.emplace_back();
    if(module.dsp.stage == module_stage::input) module_voice_start++;
    if(module.dsp.stage == module_stage::input) module_output_start++;
    if(module.dsp.stage == module_stage::voice) module_output_start++;
    module_id_to_index[module.info.tag.id] = m;
    for(int p = 0; p < module.params.size(); p++)
      param_id_to_index[module.info.tag.id][module.params[p].info.tag.id] = p;
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      param_topo_to_index[m].emplace_back();
      modules.emplace_back(module_desc(module, m, mi, module_global++, param_global));
      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        param_topo_to_index[m][mi].emplace_back();
        for(int pi = 0; pi < param.info.slot_count; pi++)
          param_topo_to_index[m][mi][p].push_back(param_global++);
      }
    }
  }

  param_global = 0;
  for(int m = 0; m < modules.size(); m++)
  {
    auto const& module = modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      param_mapping mapping;
      mapping.module_global = m;
      mapping.module_slot = module.slot;
      mapping.module_topo = module.topo;
      mapping.param_local = p;
      mapping.param_slot = param.slot;
      mapping.param_topo = param.topo;
      mapping.param_global = param_global++;
      param_index_to_tag.push_back(param.id_hash);
      param_tag_to_index[param.id_hash] = mappings.size();
      mappings.push_back(std::move(mapping));
      params.push_back(&module.params[p]);
    }
  }

  param_count = param_global;
  module_count = modules.size();
  validate_plugin_desc(*this);
  validate_plugin_topo(*plugin);
}

void
plugin_desc::init_defaults(jarray<plain_value, 4>& state) const
{
  for (int m = 0; m < plugin->modules.size(); m++)
  {
    auto const& module = plugin->modules[m];
    for(int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for(int pi = 0; pi < module.params[p].info.slot_count; pi++)
          state[m][mi][p][pi] = module.params[p].domain.default_plain();
  }
}

plugin_dims::
plugin_dims(plugin_topo const& plugin)
{
  for (int v = 0; v < plugin.polyphony; v++)
  {
    voice_module_slot.emplace_back();
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      voice_module_slot[v].push_back(module.info.slot_count);
    }
  }

  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    module_slot.push_back(module.info.slot_count);
    module_slot_param_slot.emplace_back();
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      module_slot_param_slot[m].emplace_back();
      for (int p = 0; p < module.params.size(); p++)
        module_slot_param_slot[m][mi].push_back(module.params[p].info.slot_count);
    }
  }

  validate_dims(plugin, *this);
}

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  audio = jarray<int, 1>(2, frame_count);
  for (int v = 0; v < plugin.polyphony; v++)
  {
    module_voice_cv.emplace_back();
    module_voice_audio.emplace_back();
    voices_audio.emplace_back(2, frame_count);
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      bool is_cv = module.dsp.output == module_output::cv;
      bool is_voice = module.dsp.stage == module_stage::voice;
      bool is_audio = module.dsp.output == module_output::audio;
      int cv_frames = is_cv && is_voice ? frame_count : 0;
      int audio_frames = is_audio && is_voice ? frame_count : 0;
      module_voice_cv[v].emplace_back();
      module_voice_audio[v].emplace_back();
      for (int mi = 0; mi < module.info.slot_count; mi++)
      {
        module_voice_audio[v][m].emplace_back();
        module_voice_cv[v][m].emplace_back(module.dsp.output_count, cv_frames);
        for(int oi = 0; oi < module.dsp.output_count; oi++)
          module_voice_audio[v][m][mi].emplace_back(2, audio_frames);
      }
    }
  }

  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    bool is_cv = module.dsp.output == module_output::cv;
    bool is_global = module.dsp.stage != module_stage::voice;
    bool is_audio = module.dsp.output == module_output::audio;
    int cv_frames = is_cv && is_global ? frame_count : 0;
    int audio_frames = is_audio && is_global ? frame_count : 0;
    module_global_cv.emplace_back();
    module_global_audio.emplace_back();
    accurate_automation.emplace_back();
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      accurate_automation[m].emplace_back();
      module_global_audio[m].emplace_back();
      module_global_cv[m].emplace_back(module.dsp.output_count, cv_frames);
      for(int oi = 0; oi < module.dsp.output_count; oi++)
        module_global_audio[m][mi].emplace_back(2, audio_frames);
      for (int p = 0; p < module.params.size(); p++)
      {
        int param_frames = module.params[p].dsp.rate == param_rate::accurate ? frame_count : 0;
        accurate_automation[m][mi].emplace_back(module.params[p].info.slot_count, param_frames);
      }
    }
  }

  validate_frame_dims(plugin, *this, frame_count);
}

}
