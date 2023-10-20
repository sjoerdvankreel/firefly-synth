#include <plugin_base/desc/frame_dims.hpp>

namespace plugin_base {

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int frame_count)
{
  audio = jarray<int, 1>(2, frame_count);
  for (int v = 0; v < plugin.polyphony; v++)
  {
    module_voice_cv.emplace_back();
    module_voice_audio.emplace_back();
    module_voice_scratch.emplace_back();
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
      module_voice_scratch[v].emplace_back();
      for (int mi = 0; mi < module.info.slot_count; mi++)
      {
        module_voice_audio[v][m].emplace_back();
        module_voice_cv[v][m].emplace_back(module.dsp.output_count, cv_frames);
        module_voice_scratch[v][m].emplace_back(module.dsp.scratch_count, frame_count);
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
    module_global_scratch.emplace_back();
    accurate_automation.emplace_back();
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      accurate_automation[m].emplace_back();
      module_global_audio[m].emplace_back();
      module_global_cv[m].emplace_back(module.dsp.output_count, cv_frames);
      module_global_scratch[m].emplace_back(module.dsp.scratch_count, frame_count);
      for(int oi = 0; oi < module.dsp.output_count; oi++)
        module_global_audio[m][mi].emplace_back(2, audio_frames);
      for (int p = 0; p < module.params.size(); p++)
      {
        int param_frames = module.params[p].dsp.rate == param_rate::accurate ? frame_count : 0;
        accurate_automation[m][mi].emplace_back(module.params[p].info.slot_count, param_frames);
      }
    }
  }

  validate(plugin, frame_count);
}

void
plugin_frame_dims::validate(plugin_topo const& plugin, int frame_count) const
{
  assert(audio.size() == 2);
  assert(audio[0] == frame_count);
  assert(audio[1] == frame_count);
  assert(voices_audio.size() == plugin.polyphony);
  assert(module_voice_cv.size() == plugin.polyphony);
  assert(module_voice_audio.size() == plugin.polyphony);
  assert(module_voice_scratch.size() == plugin.polyphony);

  for (int v = 0; v < plugin.polyphony; v++)
  {
    assert(voices_audio[v].size() == 2);
    assert(voices_audio[v][0] == frame_count);
    assert(voices_audio[v][1] == frame_count);
    assert(module_voice_cv[v].size() == plugin.modules.size());
    assert(module_voice_audio[v].size() == plugin.modules.size());
    assert(module_voice_scratch[v].size() == plugin.modules.size());

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
      assert(module_voice_cv[v][m].size() == module.info.slot_count);
      assert(module_voice_audio[v][m].size() == module.info.slot_count);
      assert(module_voice_scratch[v][m].size() == module.info.slot_count);
      for (int mi = 0; mi < module.info.slot_count; mi++)
      {
        assert(module_voice_cv[v][m][mi].size() == module.dsp.output_count);
        assert(module_voice_audio[v][m][mi].size() == module.dsp.output_count);
        assert(module_voice_scratch[v][m][mi].size() == module.dsp.scratch_count);

        for (int oi = 0; oi < module.dsp.scratch_count; oi++)
          assert(module_voice_scratch[v][m][mi][oi] == frame_count);
        for(int oi = 0; oi < module.dsp.output_count; oi++)
        {
          assert(module_voice_cv[v][m][mi][oi] == cv_frames);
          assert(module_voice_audio[v][m][mi][oi].size() == 2);
          for(int c = 0; c < 2; c++)
            assert(module_voice_audio[v][m][mi][oi][c] == audio_frames);
        }
      }
    }
  }

  assert(module_global_cv.size() == plugin.modules.size());
  assert(module_global_audio.size() == plugin.modules.size());
  assert(module_global_scratch.size() == plugin.modules.size());
  assert(accurate_automation.size() == plugin.modules.size());
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
    assert(module_global_cv[m].size() == module.info.slot_count);
    assert(module_global_audio[m].size() == module.info.slot_count);
    assert(module_global_scratch[m].size() == module.info.slot_count);
    assert(accurate_automation[m].size() == module.info.slot_count);

    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(module_global_cv[m][mi].size() == module.dsp.output_count);
      assert(module_global_audio[m][mi].size() == module.dsp.output_count);
      assert(module_global_scratch[m][mi].size() == module.dsp.scratch_count);
      for (int si = 0; si < module.dsp.scratch_count; si++)
        assert(module_global_scratch[m][mi][si] == frame_count);
      for(int oi = 0; oi < module.dsp.output_count; oi++)
      {
        assert(module_global_cv[m][mi][oi] == cv_frames);
        assert(module_global_audio[m][mi][oi].size() == 2);
        for(int c = 0; c < 2; c++)
          assert(module_global_audio[m][mi][oi][c] == audio_frames);
      }

      assert(accurate_automation[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        int accurate_frames = param.dsp.rate == param_rate::accurate? frame_count: 0;
        (void)accurate_frames;
        assert(accurate_automation[m][mi][p].size() == param.info.slot_count);
        for(int pi = 0; pi < param.info.slot_count; pi++)
          assert(accurate_automation[m][mi][p][pi] == accurate_frames);
      }
    }
  }
}

}
