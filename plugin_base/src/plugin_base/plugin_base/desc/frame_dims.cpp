#include <plugin_base/desc/frame_dims.hpp>

namespace plugin_base {

plugin_frame_dims::
plugin_frame_dims(plugin_topo const& plugin, int polyphony, int frame_count)
{
  audio = jarray<int, 1>(2, frame_count);
  for (int v = 0; v < polyphony; v++)
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
        module_voice_cv[v][m].emplace_back();
        module_voice_audio[v][m].emplace_back();
        module_voice_scratch[v][m].emplace_back(module.dsp.scratch_count, frame_count);
        for (int o = 0; o < module.dsp.outputs.size(); o++)
        {
          module_voice_audio[v][m][mi].emplace_back();
          module_voice_cv[v][m][mi].emplace_back(module.dsp.outputs[o].info.slot_count, cv_frames);
          for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
            module_voice_audio[v][m][mi][o].emplace_back(2, audio_frames);
        }
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
    midi_automation.emplace_back();
    module_global_cv.emplace_back();
    module_global_audio.emplace_back();
    module_global_scratch.emplace_back();
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      module_global_cv[m].emplace_back();
      module_global_audio[m].emplace_back();
      midi_automation[m].emplace_back(module.midi_sources.size(), frame_count);
      module_global_scratch[m].emplace_back(module.dsp.scratch_count, frame_count);
      for (int o = 0; o < module.dsp.outputs.size(); o++)
      {
        module_global_audio[m][mi].emplace_back();
        module_global_cv[m][mi].emplace_back(module.dsp.outputs[o].info.slot_count, cv_frames);
        for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
          module_global_audio[m][mi][o].emplace_back(2, audio_frames);
      }
    }
  }

  validate(plugin, polyphony, frame_count);
}

void
plugin_frame_dims::validate(plugin_topo const& plugin, int polyphony, int frame_count) const
{
  assert(audio.size() == 2);
  assert(audio[0] == frame_count);
  assert(audio[1] == frame_count);
  assert(voices_audio.size() == polyphony);
  assert(module_voice_cv.size() == polyphony);
  assert(module_voice_audio.size() == polyphony);
  assert(module_voice_scratch.size() == polyphony);

  for (int v = 0; v < polyphony; v++)
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
        assert(module_voice_cv[v][m][mi].size() == module.dsp.outputs.size());
        assert(module_voice_audio[v][m][mi].size() == module.dsp.outputs.size());
        assert(module_voice_scratch[v][m][mi].size() == module.dsp.scratch_count);

        for (int s = 0; s < module.dsp.scratch_count; s++)
          assert(module_voice_scratch[v][m][mi][s] == frame_count);
        for(int o = 0; o < module.dsp.outputs.size(); o++)
          for(int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
          {
            assert(module_voice_cv[v][m][mi][o][oi] == cv_frames);
            assert(module_voice_audio[v][m][mi][o][oi].size() == 2);
            for(int c = 0; c < 2; c++)
              assert(module_voice_audio[v][m][mi][o][oi][c] == audio_frames);
        }
      }
    }
  }

  assert(midi_automation.size() == plugin.modules.size());
  assert(module_global_cv.size() == plugin.modules.size());
  assert(module_global_audio.size() == plugin.modules.size());
  assert(module_global_scratch.size() == plugin.modules.size());
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
    assert(midi_automation[m].size() == module.info.slot_count);
    assert(module_global_cv[m].size() == module.info.slot_count);
    assert(module_global_audio[m].size() == module.info.slot_count);
    assert(module_global_scratch[m].size() == module.info.slot_count);

    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(module_global_cv[m][mi].size() == module.dsp.outputs.size());
      assert(module_global_audio[m][mi].size() == module.dsp.outputs.size());
      assert(module_global_scratch[m][mi].size() == module.dsp.scratch_count);
      assert(midi_automation[m][mi].size() == module.midi_sources.size());
      for (int ms = 0; ms < module.midi_sources.size(); ms++)
        assert(midi_automation[m][mi][ms] == frame_count);
      for (int s = 0; s < module.dsp.scratch_count; s++)
        assert(module_global_scratch[m][mi][s] == frame_count);
      for (int o = 0; o < module.dsp.outputs.size(); o++)
        for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
        {
          assert(module_global_cv[m][mi][o][oi] == cv_frames);
          assert(module_global_audio[m][mi][o][oi].size() == 2);
          for(int c = 0; c < 2; c++)
            assert(module_global_audio[m][mi][o][oi][c] == audio_frames);
        }
    }
  }
}

}
