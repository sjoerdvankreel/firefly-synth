#if JUCE_LINUX || JUCE_BSD
#include <plugin_base.vst3/inf_editor_linux.hpp>

#include <pluginterfaces/vst/ivstplugview.h>
#include <juce_events/native/juce_EventLoopInternal_linux.h>
#include <juce_audio_plugin_client/detail/juce_LinuxMessageThread.h>

using namespace Steinberg;
#include <plugin_base.vst3/inf_editor_linux_juce_vst3_common_rip.hpp>
#include <plugin_base.vst3/inf_editor_linux_juce_audio_plugin_client_vst3_rip.hpp>

#include <iostream>

using namespace juce;

namespace plugin_base::vst3 {

struct inf_editor_linux::impl
{
  std::unique_ptr<plugin_gui> gui;
  SharedResourcePointer<EventHandler> event_handler;
  SharedResourcePointer<detail::MessageThread> message_thread;

  ~impl() { std::cout << "Do i even destruct?\n"; }
};

inf_editor_linux::
~inf_editor_linux()
{
  std::cout << "D1\n";
}

inf_editor_linux::
inf_editor_linux(inf_controller* controller):
inf_editor(controller), _impl()
{
  std::cout << "C1\n";
  MessageManager::getInstance();
  MessageManagerLock const mm_lock;
  std::cout << "i really have the lock ctor and the msgmgr\n";
  std::cout << "C2\n";
  _impl.reset(new impl());
  _impl->gui.reset(new plugin_gui(&controller->desc(), &controller->ui_state()));
  std::cout << "C3\n";
  std::cout << "i dont really have the lock ctor but still the mgr\n";
}

plugin_gui* 
inf_editor_linux::gui() const
{ 
  assert(_impl->gui.get());
  return _impl->gui.get(); 
}

tresult PLUGIN_API 
inf_editor_linux::removed()
{
  std::cout << "R1\n";
  _impl->gui->remove_ui_listener(_controller);
  std::cout << "R2\n";
  _impl->gui->setVisible(false);
  std::cout << "R3\n";
  _impl->gui->removeFromDesktop();
  std::cout << "R4\n";
  _impl->event_handler->unregisterHandlerForFrame(plugFrame);
  std::cout << "R5\n";
  return EditorView::removed();
}

tresult PLUGIN_API 
inf_editor_linux::attached(void* parent, FIDString type)
{
  std::cout << "A1\n";
  _impl->event_handler->registerHandlerForFrame(plugFrame);
  std::cout << "A2\n";
  _impl->gui->addToDesktop(0, parent);
  std::cout << "A3\n";
  _impl->gui->setVisible(true);
  std::cout << "A4\n";
  _impl->gui->add_ui_listener(_controller);
  std::cout << "A5\n";
  return kResultTrue;
}

}

#endif