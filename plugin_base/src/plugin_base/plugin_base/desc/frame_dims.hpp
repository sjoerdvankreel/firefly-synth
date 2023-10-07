#pragma once

#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

// runtime plugin dsp dimensions
struct plugin_frame_dims final {
  jarray<int, 1> audio;
  jarray<int, 2> voices_audio;
  jarray<int, 4> module_voice_cv;
  jarray<int, 3> module_global_cv;
  jarray<int, 5> module_voice_audio;
  jarray<int, 4> module_global_audio;
  jarray<int, 4> accurate_automation;

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_frame_dims);
  plugin_frame_dims(plugin_topo const& plugin, int frame_count);
  void validate(plugin_topo const& plugin, int frame_count) const;
};

}
