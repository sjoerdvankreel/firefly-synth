#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>

#include <set>

namespace plugin_base {

static std::string
param_id(param_topo const& param, int slot)
{
  std::string result = param.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
module_id(module_topo const& module, int slot)
{
  std::string result = module.id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
param_name(param_topo const& param, int slot)
{
  std::string result = param.name;
  if (param.slot_count > 1) result += " " + std::to_string(slot + 1);
  return result;
}

static std::string
module_name(module_topo const& module, int slot)
{
  std::string result = module.name;
  if (module.slot_count > 1) result += " " + std::to_string(slot + 1);
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
  assert(dims.params.size() == plugin.modules.size());
  assert(dims.modules.size() == plugin.modules.size());
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    assert(dims.modules[m] == module.slot_count);
    assert(dims.params[m].size() == module.slot_count);
    for (int mi = 0; mi < module.slot_count; mi++)
    {
      assert(dims.params[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
        assert(dims.params[m][mi][p] == module.params[m].slot_count);
    }
  }
}

static void
validate_frame_dims(
  plugin_topo const& plugin, plugin_frame_dims const& dims, int frame_count)
{
  assert(dims.voice_cv.size() == plugin.polyphony);
  assert(dims.voice_audio.size() == plugin.polyphony);
  for (int v = 0; v < plugin.polyphony; v++)
  {
    assert(dims.voice_cv[v].size() == plugin.modules.size());
    assert(dims.voice_audio[v].size() == plugin.modules.size());
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      bool is_cv = module.output == module_output::cv;
      bool is_voice = module.scope == module_scope::voice;
      bool is_audio = module.output == module_output::audio;
      int cv_frames = is_cv && is_voice ? frame_count : 0;
      int audio_frames = is_audio && is_voice ? frame_count : 0;
      assert(dims.voice_cv[v][m].size() == module.slot_count);
      assert(dims.voice_audio[v][m].size() == module.slot_count);
      for (int mi = 0; mi < module.slot_count; mi++)
      {
        assert(dims.voice_cv[v][m][mi] == cv_frames);
        assert(dims.voice_audio[v][m][mi].size() == 2);
        for(int c = 0; c < 2; c++)
          assert(dims.voice_audio[v][m][mi][c] == audio_frames);
      }
    }
  }

  assert(dims.accurate.size() == plugin.modules.size());
  assert(dims.global_cv.size() == plugin.modules.size());
  assert(dims.global_audio.size() == plugin.modules.size());
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    bool is_cv = module.output == module_output::cv;
    bool is_audio = module.output == module_output::audio;
    bool is_global = module.scope == module_scope::global;
    int cv_frames = is_cv && is_global ? frame_count : 0;
    int audio_frames = is_audio && is_global ? frame_count : 0;
    assert(dims.accurate[m].size() == module.slot_count);
    assert(dims.global_cv[m].size() == module.slot_count);
    assert(dims.global_audio[m].size() == module.slot_count);

    for (int mi = 0; mi < module.slot_count; mi++)
    {
      assert(dims.global_cv[m][mi] == cv_frames);
      assert(dims.global_audio[m][mi].size() == 2);
      for(int c = 0; c < 2; c++)
        assert(dims.global_audio[m][mi][c] == audio_frames);

      assert(dims.accurate[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        int accurate_frames = param.rate == param_rate::accurate? frame_count: 0;
        assert(dims.accurate[m][mi][p].size() == param.slot_count);
        for(int pi = 0; pi < param.slot_count; pi++)
          assert(dims.accurate[m][mi][p][pi] == accurate_frames);
      }
    }
  }
}

static void
validate_module_desc(plugin_desc const& plugin_desc, module_desc const& desc)
{
  assert(desc.module);
  assert(desc.id.size());
  assert(desc.name.size());
  assert(desc.params.size());
  assert(desc.id_hash >= 0);
  assert(0 <= desc.slot && desc.slot < desc.module->slot_count);
  assert(0 <= desc.global && desc.global < plugin_desc.modules.size());
  assert(0 <= desc.topo && desc.topo < plugin_desc.plugin->modules.size());
}

static void 
validate_param_desc(module_desc const& module_desc, param_desc const& desc)
{
  assert(desc.param);
  assert(desc.id_hash >= 0);
  assert(desc.id.size() > 0);
  assert(0 < desc.short_name.size() && desc.short_name.size() < desc.full_name.size());
  assert(0 <= desc.slot && desc.slot < desc.param->slot_count);
  assert(0 <= desc.local && desc.local < module_desc.params.size());
  assert(0 <= desc.topo && desc.topo < module_desc.module->params.size());
}

static void
validate_section_topo(module_topo const& module_topo, section_topo const& topo)
{
  assert(topo.name.size());
  assert(0 <= topo.section && topo.section < module_topo.sections.size());
}

static void
validate_module_topo(module_topo const& topo)
{
  assert(topo.id.size());
  assert(topo.name.size());
  assert(topo.params.size());
  assert(topo.slot_count > 0);
  assert(topo.engine_factory);
  assert(0 < topo.sections.size() && topo.sections.size() <= topo.params.size());
}

static void
validate_param_topo(module_topo const& module_topo, param_topo const& topo)
{
  assert(topo.id.size());
  assert(topo.name.size());
  assert(topo.section >= 0);
  assert(topo.max > topo.min);
  assert(topo.default_.size());
  assert(topo.section < module_topo.sections.size());
  assert(0 < topo.slot_count && topo.slot_count <= 1024);
  assert(0 <= topo.section && topo.section < module_topo.sections.size());

  assert(topo.type != param_type::log || topo.exp != 0);
  assert(topo.type == param_type::log || topo.exp == 0);
  assert(topo.dir != param_dir::output || topo.rate == param_rate::block);
  
  assert(topo.edit != param_edit::toggle || topo.min == 0);
  assert(topo.edit != param_edit::toggle || topo.max == 1);
  assert(topo.edit != param_edit::toggle || topo.type == param_type::step);

  assert(topo.type == param_type::name || topo.names.size() == 0);
  assert(topo.type != param_type::name || topo.min == 0);
  assert(topo.type != param_type::name || topo.names.size() > 0);
  assert(topo.type != param_type::name || topo.unit.size() == 0);
  assert(topo.type != param_type::name || topo.max == topo.names.size() - 1);

  assert(topo.type == param_type::item || topo.items.size() == 0);
  assert(topo.type != param_type::item || topo.items.size() > 0);
  assert(topo.type != param_type::item || topo.unit.size() == 0);
  assert(topo.type != param_type::item || topo.min == 0);
  assert(topo.type != param_type::item || topo.max == topo.items.size() - 1);

  assert(topo.display != param_display::pct || topo.unit == "%");
  assert(topo.display == param_display::normal || topo.type == param_type::linear);

  assert(topo.is_real() || (int)topo.min == topo.min);
  assert(topo.is_real() || (int)topo.max == topo.max);
  assert(topo.is_real() || topo.rate == param_rate::block);
  assert(topo.is_real() || topo.display == param_display::normal);
  assert(topo.is_real() || topo.min <= topo.default_plain().step());
  assert(topo.is_real() || topo.max >= topo.default_plain().step());
  assert(!topo.is_real() || topo.min <= topo.default_plain().real());
  assert(!topo.is_real() || topo.max >= topo.default_plain().real());
  assert(!topo.is_real() || topo.display == param_display::pct_no_unit || topo.unit.size() > 0);
}

static void
validate_plugin_desc(plugin_desc const& desc)
{
  int param_global = 0;
  std::set<int> all_hashes;
  std::set<std::string> all_ids;
  (void)param_global;

  assert(desc.param_count > 0);
  assert(desc.module_count > 0);
  assert(desc.modules.size() == desc.module_count);
  assert(desc.mappings.size() == desc.param_count);
  assert(desc.param_tag_to_index.size() == desc.param_count);
  assert(desc.param_index_to_tag.size() == desc.param_count);
  assert(desc.param_id_to_index.size() == desc.plugin->modules.size());
  assert(desc.module_id_to_index.size() == desc.plugin->modules.size());

  for(int m = 0; m < desc.plugin->modules.size(); m++)
  {
    auto const& module = desc.plugin->modules[m];
    (void)module;
    assert(desc.param_id_to_index.at(module.id).size() == module.params.size());
  }

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

static void
validate_plugin_topo(plugin_topo const& topo)
{
  std::set<std::string> param_ids;
  std::set<std::string> module_ids;

  assert(topo.id.size());
  assert(topo.name.size());
  assert(topo.modules.size());
  assert(topo.version_major >= 0);
  assert(topo.version_minor >= 0);
  assert(topo.polyphony >= 0 && topo.polyphony <= 1024);
  assert(0 < topo.gui_default_width && topo.gui_default_width <= 3840);
  assert(0 < topo.gui_aspect_ratio && topo.gui_aspect_ratio <= 21.0 / 9.0);

  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    validate_module_topo(module);
    INF_ASSERT_EXEC(module_ids.insert(module.id).second);
    for (int s = 0; s < module.sections.size(); s++)
      validate_section_topo(module, module.sections[s]);
    for (int p = 0; p < module.params.size(); p++)
    {
      validate_param_topo(module, module.params[p]);
      INF_ASSERT_EXEC(param_ids.insert(module.params[p].id).second);
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
  short_name = param_name(param_, slot);
  full_name = module_name(module_, module_slot) + " " + short_name;
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
    for(int i = 0; i < module_.params[p].slot_count; i++)
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
    module_id_to_index[module.id] = m;
    for(int p = 0; p < module.params.size(); p++)
      param_id_to_index[module.id][module.params[p].id] = p;
    for(int mi = 0; mi < module.slot_count; mi++)
    {
      modules.emplace_back(module_desc(module, m, mi, module_global++, param_global));
      for(int p = 0; p < module.params.size(); p++)
        param_global += module.params[p].slot_count;
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
    for(int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for(int pi = 0; pi < module.params[p].slot_count; pi++)
          state[m][mi][p][pi] = module.params[p].default_plain();
  }
}

plugin_dims::
plugin_dims(plugin_topo const& plugin)
{
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    modules.push_back(module.slot_count);
    params.emplace_back();
    for(int mi = 0; mi < module.slot_count; mi++)
    {
      params[m].emplace_back();
      for (int p = 0; p < module.params.size(); p++)
        params[m][mi].push_back(module.params[p].slot_count);
    }
  }
  validate_dims(plugin, *this);
}

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  for (int v = 0; v < plugin.polyphony; v++)
  {
    voice_cv.emplace_back();
    voice_audio.emplace_back();
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      bool is_cv = module.output == module_output::cv;
      bool is_audio = module.output == module_output::audio;
      bool is_voice = module.scope == module_scope::voice;
      int cv_frames = is_cv && is_voice ? frame_count : 0;
      int audio_frames = is_audio && is_voice ? frame_count : 0;
      voice_cv[v].emplace_back(module.slot_count, cv_frames);
      for (int mi = 0; mi < module.slot_count; mi++)
        voice_audio[v][m].emplace_back(2, audio_frames);
    }
  }

  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    bool is_cv = module.output == module_output::cv;
    bool is_audio = module.output == module_output::audio;
    bool is_global = module.scope == module_scope::global;
    int cv_frames = is_cv && is_global ? frame_count : 0;
    int audio_frames = is_audio && is_global ? frame_count : 0;
    accurate.emplace_back();
    global_audio.emplace_back();
    global_cv.emplace_back(module.slot_count, cv_frames);
    for (int mi = 0; mi < module.slot_count; mi++)
    {
      accurate[m].emplace_back();
      global_audio[m].emplace_back(2, audio_frames);
      for (int p = 0; p < module.params.size(); p++)
      {
        int param_frames = module.params[p].rate == param_rate::accurate ? frame_count : 0;
        accurate[m][mi].emplace_back(module.params[p].slot_count, param_frames);
      }
    }
  }

  validate_frame_dims(plugin, *this, frame_count);
}

}
