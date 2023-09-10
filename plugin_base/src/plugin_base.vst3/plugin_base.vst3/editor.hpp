#pragma once
#include <plugin_base/gui.hpp>
#include <plugin_base.vst3/controller.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class editor final:
public Steinberg::Vst::EditorView,
public plugin_base::any_param_ui_listener 
{
  std::unique_ptr<plugin_gui> _gui = {};
  plugin_base::vst3::controller* const _controller;
public: 
  editor(plugin_base::vst3::controller* controller, plugin_topo_factory factory);

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;

  void ui_param_changing(int param_index, param_value value) override;
  void plugin_param_changed(int param_index, param_value value) { _gui->plugin_param_changed(param_index, value); }
  void ui_param_end_changes(int param_index) override { _controller->endEdit(_controller->desc().index_to_id[param_index]); }
  void ui_param_begin_changes(int param_index) override { _controller->beginEdit(_controller->desc().index_to_id[param_index]); }

  Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override { return Steinberg::kResultTrue; }
};

}
