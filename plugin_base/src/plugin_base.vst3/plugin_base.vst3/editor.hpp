#pragma once
#include <plugin_base/desc.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class editor final:
public Steinberg::Vst::EditorView {
  plugin_desc const _desc;
public: 
  editor(Steinberg::Vst::EditController* controller, plugin_topo_factory factory) : EditorView(controller), _desc(factory) {}
};

}
