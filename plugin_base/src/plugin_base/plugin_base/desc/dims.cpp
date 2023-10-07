#include <plugin_base/desc/dims.hpp>

namespace plugin_base {

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
