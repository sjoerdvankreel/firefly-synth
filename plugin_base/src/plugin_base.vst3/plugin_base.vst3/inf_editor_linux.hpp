#pragma once

#include <plugin_base.vst3/inf_editor.hpp>

namespace plugin_base::vst3 {

class inf_editor_linux final:
public inf_editor
{
  struct impl;
  std::unique_ptr<impl> _impl;
protected:
  plugin_gui* gui() const override;

public: 
  INF_DECLARE_MOVE_ONLY(inf_editor_linux);
  inf_editor_linux(inf_controller* controller);

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
};

}
