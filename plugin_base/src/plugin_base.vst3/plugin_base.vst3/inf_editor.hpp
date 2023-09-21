#pragma once

#include <plugin_base/gui.hpp>
#include <plugin_base.vst3/inf_controller.hpp>

#include <public.sdk/source/vst/vsteditcontroller.h>
#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>

#include <utility>

namespace plugin_base::vst3 {

class inf_editor:
public Steinberg::Vst::EditorView,
public Steinberg::IPlugViewContentScaleSupport
{
protected:
  virtual plugin_gui* gui() const = 0;
  inf_controller* const _controller = {};
  inf_editor(inf_controller* controller): 
  EditorView(controller), _controller(controller) {}

public: 
  INF_DECLARE_MOVE_ONLY(inf_editor);
  void plugin_changed(int index, plain_value plain) { gui()->plugin_changed(index, plain); }

  Steinberg::uint32 PLUGIN_API addRef() override { return EditorView::addRef(); }
  Steinberg::uint32 PLUGIN_API release() override { return EditorView::release(); } 
  Steinberg::tresult PLUGIN_API setContentScaleFactor(float factor) override;
  Steinberg::tresult PLUGIN_API queryInterface(Steinberg::TUID const iid, void** obj) override;

  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
  Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override  { return Steinberg::kResultTrue; }
};

}
