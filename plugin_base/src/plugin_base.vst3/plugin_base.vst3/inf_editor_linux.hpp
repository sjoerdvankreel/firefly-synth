#pragma once

#include <iostream>
#include <plugin_base.vst3/inf_editor.hpp>
#include <juce_events/juce_events.h>

namespace plugin_base::vst3 {

  struct MessageManagerLockedDeleter
  {
    template <typename ObjectType>
    void operator() (ObjectType* object) const noexcept
    {
      std::cout << "i dont really have the lock dtor\n";

      std::cout << "D1\n";
      const juce::MessageManagerLock mmLock;
      std::cout << "D2\n";
      delete object;
      std::cout << "D3\n";

      std::cout << "i dont really have the lock dtor\n";

    }
  };

class inf_editor_linux final:
public inf_editor
{
  struct impl;
  std::unique_ptr<impl, MessageManagerLockedDeleter> _impl;
protected:
  plugin_gui* gui() const override;

public: 
  ~inf_editor_linux();
  INF_DECLARE_MOVE_ONLY(inf_editor_linux);
  inf_editor_linux(inf_controller* controller);

  Steinberg::tresult PLUGIN_API removed() override;
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
};

}
