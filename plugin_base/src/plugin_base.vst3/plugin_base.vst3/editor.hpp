#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base.vst3/controller.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class editor final:
public Steinberg::Vst::EditorView {
  plugin_base::vst3::controller* const _controller;
public: 
  editor(plugin_base::vst3::controller* controller) : 
  EditorView(controller), _controller(controller) {}
};

}
