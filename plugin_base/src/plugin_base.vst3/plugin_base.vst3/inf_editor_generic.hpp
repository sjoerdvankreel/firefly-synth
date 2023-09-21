#pragma once

#include <plugin_base.vst3/inf_editor.hpp>

namespace plugin_base::vst3 {

class inf_editor_generic final:
public inf_editor
{
  std::unique_ptr<plugin_gui> _gui = {};
protected:
  plugin_gui* gui() const override { return _gui.get(); }

public: 
  INF_DECLARE_MOVE_ONLY(inf_editor_generic);
  inf_editor_generic(inf_controller* controller);

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
};

}
