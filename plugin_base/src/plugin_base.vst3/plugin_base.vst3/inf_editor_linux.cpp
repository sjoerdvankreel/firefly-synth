#if JUCE_LINUX || JUCE_BSD
#include <plugin_base.vst3/inf_editor_linux.hpp>

#include <pluginterfaces/vst/ivstplugview.h>
#include <juce_events/native/juce_EventLoopInternal_linux.h>
#include <juce_audio_plugin_client/detail/juce_LinuxMessageThread.h>

using namespace Steinberg;
#include <plugin_base.vst3/inf_editor_linux_juce_vst3_common_rip.hpp>
#include <plugin_base.vst3/inf_editor_linux_juce_audio_plugin_client_vst3_rip.hpp>

using namespace juce;

namespace plugin_base::vst3 {

struct inf_editor_linux::impl
{
  SharedResourcePointer<EventHandler> event_handler;
  SharedResourcePointer<detail::MessageThread> message_thread;
  std::unique_ptr<plugin_gui, MessageManagerLockedDeleter> gui;
};

inf_editor_linux::
inf_editor_linux(inf_controller* controller):
inf_editor(controller), _impl(std::make_unique<impl>())
{ 
  const MessageManagerLock mm_lock;
  _impl->gui.reset(new plugin_gui(&controller->desc(), &controller->ui_state()));
}

plugin_gui* 
inf_editor_linux::gui() const
{ return _impl->gui.get(); }

tresult PLUGIN_API 
inf_editor_linux::removed()
{
  _impl->gui->remove_ui_listener(_controller);
  _impl->gui->setVisible(false);
  _impl->event_handler->unregisterHandlerForFrame(plugFrame);
  return EditorView::removed();
}

tresult PLUGIN_API 
inf_editor_linux::attached(void* parent, FIDString type)
{
  _impl->event_handler->registerHandlerForFrame(plugFrame);
  _impl->gui->addToDesktop(0, parent);
  _impl->gui->setVisible(true);
  _impl->gui->add_ui_listener(_controller);
  return EditorView::attached(parent, type);
}

}

#endif