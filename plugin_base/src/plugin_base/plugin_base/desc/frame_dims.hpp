#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

namespace plugin_base {

// runtime plugin dsp dimensions
struct plugin_frame_dims final {
  jarray<int, 1> audio;
  jarray<int, 2> voices_audio;
  jarray<int, 5> module_voice_cv;
  jarray<int, 4> module_global_cv;
  jarray<int, 6> module_voice_audio;
  jarray<int, 5> module_global_audio;
  jarray<int, 3> midi_automation;
  jarray<int, 4> accurate_automation;
  jarray<int, 4> module_voice_scratch;
  jarray<int, 3> module_global_scratch;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_frame_dims);
  plugin_frame_dims(plugin_topo const& plugin, int polyphony, int frame_count);
  void validate(plugin_topo const& plugin, int polyphony, int frame_count) const;
};

}
