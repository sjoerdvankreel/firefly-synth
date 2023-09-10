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
  plugin_gui _gui;
  plugin_base::vst3::controller* const _controller;
public: 
  editor(plugin_base::vst3::controller* controller, plugin_topo_factory factory) : 
  EditorView(controller), _gui(factory), _controller(controller) {}

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* new_size) override;
  Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;

  void plugin_param_changed(int param_index, param_value value) { _gui.plugin_param_changed(param_index, value); }
  void ui_value_changed(int param_index, param_value value) override { _controller->force_update_value(param_index, value); }

  Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override { return Steinberg::kResultTrue; }
};

}
