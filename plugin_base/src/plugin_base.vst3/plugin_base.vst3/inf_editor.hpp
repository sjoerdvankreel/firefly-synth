#pragma once

#include <plugin_base/gui.hpp>
#include <plugin_base.vst3/inf_controller.hpp>

#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class inf_editor final:
public Steinberg::Vst::EditorView 
{
  inf_controller* const _controller = {};
  std::unique_ptr<plugin_gui> _gui = {};

public: 
  INF_DECLARE_MOVE_ONLY(inf_editor);
  inf_editor(inf_controller* controller);
  void plugin_changed(int index, plain_value plain) { _gui->plugin_changed(index, plain); }

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
  Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override  { return Steinberg::kResultTrue; }
};

}
