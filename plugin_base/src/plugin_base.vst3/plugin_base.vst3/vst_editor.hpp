#pragma once

#include <plugin_base/gui.hpp>
#include <plugin_base.vst3/vst_controller.hpp>

#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class vst_editor final:
public Steinberg::Vst::EditorView, public ui_listener 
{
  std::unique_ptr<plugin_gui> _gui = {};
  vst_controller* const _controller = {};
public: 
  vst_editor(vst_controller* controller);

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
  Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override  { return Steinberg::kResultTrue; }

  void ui_changing(int index, plain_value plain) override;
  void plugin_param_changed(int index, plain_value plain) { _gui->plugin_changed(index, plain); }
  void ui_end_changes(int index) override { _controller->endEdit(_controller->desc().index_to_id[index]); }
  void ui_begin_changes(int index) override { _controller->beginEdit(_controller->desc().index_to_id[index]); }
};

}
